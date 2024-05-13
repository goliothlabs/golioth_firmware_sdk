/*
 * Copyright (c) 2024 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <golioth/client.h>
#include <string.h>
#include <assert.h>
#include "coap_client.h"
#include "coap_blockwise.h"

#define IS_VALID_POWER_OF_TWO(x) (((x) & ((x) -1)) == 0)

#define IS_VALID_BLOCK_SIZE(x) ((x) >= 16 && (x) <= 1024 && IS_VALID_POWER_OF_TWO(x))

LOG_TAG_DEFINE(blockwise_module);

struct blockwise_transfer
{
    bool is_last;
    enum golioth_status status;
    enum golioth_content_type content_type;
    int retry_count;
    uint32_t offset;
    size_t block_size;
    const char *path_prefix;
    const char *path;
    uint8_t *block_buffer;
    union
    {
        read_block_cb read_cb;
        write_block_cb write_cb;
    } callback;
    void *callback_arg;
};

/* Blockwise Uploads related functions */

// Function to initialize the blockwise_transfer structure for uploads
static void blockwise_upload_init(struct blockwise_transfer *ctx,
                                  uint8_t *data_buf,
                                  const char *path_prefix,
                                  const char *path,
                                  enum golioth_content_type content_type,
                                  read_block_cb cb,
                                  void *callback_arg)
{
    ctx->is_last = false;
    ctx->path_prefix = path_prefix;
    ctx->path = path;
    ctx->content_type = content_type;
    ctx->retry_count = 0;
    ctx->block_size = 0;
    ctx->offset = 0;
    ctx->block_buffer = data_buf;
    ctx->callback_arg = callback_arg;
    ctx->callback.read_cb = cb;
}

// Blockwise upload's internal callback function that the COAP client calls
static void on_block_sent(struct golioth_client *client,
                          const struct golioth_response *response,
                          const char *path,
                          void *arg)
{
    assert(arg);
    struct blockwise_transfer *ctx = (struct blockwise_transfer *) arg;
    ctx->status = response->status;
}

// Function to call the application's read block callback for obtaining
// blockwise upload data
static enum golioth_status call_read_block_callback(struct blockwise_transfer *ctx)
{
    size_t block_size = 0;
    enum golioth_status status = ctx->callback.read_cb(ctx->offset,
                                                       ctx->block_buffer,
                                                       &block_size,
                                                       &ctx->is_last,
                                                       ctx->callback_arg);
    if (status != GOLIOTH_OK)
    {
        return status;
    }

    if (block_size == 0 || block_size > CONFIG_GOLIOTH_BLOCKWISE_UPLOAD_BLOCK_SIZE)
    {
        return GOLIOTH_ERR_INVALID_BLOCK_SIZE;
    }

    if (ctx->is_last)
    {
        ctx->block_size = block_size;
    }
    else if (ctx->block_size == 0)
    {
        // This is the first time we called ctx->callback.read_cb()
        // Make sure the block size is a valid size for blockwise uploads
        if (!IS_VALID_BLOCK_SIZE(block_size))
        {
            return GOLIOTH_ERR_INVALID_BLOCK_SIZE;
        }
        ctx->block_size = block_size;
    }
    else if (block_size != ctx->block_size)
    {
        // Size of all blocks must be the same
        return GOLIOTH_ERR_INVALID_BLOCK_SIZE;
    }

    return GOLIOTH_OK;
}

// Function to upload a single block
static void upload_single_block(struct golioth_client *client, struct blockwise_transfer *ctx)
{
    enum golioth_status status = golioth_coap_client_set_block(client,
                                                               ctx->path_prefix,
                                                               ctx->path,
                                                               ctx->is_last,
                                                               ctx->content_type,
                                                               ctx->offset,
                                                               ctx->block_buffer,
                                                               ctx->block_size,
                                                               on_block_sent,
                                                               ctx,
                                                               true,
                                                               GOLIOTH_SYS_WAIT_FOREVER);
    ctx->status = status;
}

// Function to handle blockwise upload errors
// This function retries for all errors like invalid state, failed mem alloc,
// queue full, disconnects, etc returned by golioth_coap_client_set_block
static enum golioth_status handle_upload_error(struct golioth_client *client,
                                               struct blockwise_transfer *ctx)
{
    while (1)
    {
        GLTH_LOGE(TAG, "Blockwise upload failed with error %d. Retrying.", ctx->status);
        golioth_sys_msleep(CONFIG_GOLIOTH_BLOCKWISE_RETRY_SLEEP_TIME_MS);
        upload_single_block(client, ctx);
        if (ctx->status == GOLIOTH_OK)
        {
            ctx->retry_count = 0;
            break;
        }
        else
        {
            if (CONFIG_GOLIOTH_BLOCKWISE_RETRY_COUNT != -1)
            {
                ctx->retry_count++;
                if (ctx->retry_count > CONFIG_GOLIOTH_BLOCKWISE_RETRY_COUNT)
                {
                    break;
                }
            }
        }
    }
    return ctx->status;
}

// Function to manage blockwise upload and handle errors
static enum golioth_status process_blockwise_uploads(struct golioth_client *client,
                                                     struct blockwise_transfer *ctx)
{
    enum golioth_status status = call_read_block_callback(ctx);
    if (status == GOLIOTH_OK)
    {
        upload_single_block(client, ctx);
        if (ctx->status == GOLIOTH_OK)
        {
            ctx->offset++;
        }
        else
        {
            status = handle_upload_error(client, ctx);
        }
    }
    return status;
}

enum golioth_status golioth_blockwise_post(struct golioth_client *client,
                                           const char *path_prefix,
                                           const char *path,
                                           enum golioth_content_type content_type,
                                           read_block_cb cb,
                                           void *callback_arg)
{
    enum golioth_status status;

    if (!client || !path || !cb)
    {
        return GOLIOTH_ERR_NULL;
    }

    if (!path_prefix)
    {
        path_prefix = "";
    }

    struct blockwise_transfer *ctx = malloc(sizeof(struct blockwise_transfer));
    if (!ctx)
    {
        return GOLIOTH_ERR_MEM_ALLOC;
    }

    uint8_t *data_buff = malloc(CONFIG_GOLIOTH_BLOCKWISE_UPLOAD_BLOCK_SIZE);
    if (!data_buff)
    {
        free(ctx);
        return GOLIOTH_ERR_MEM_ALLOC;
    }

    blockwise_upload_init(ctx, data_buff, path_prefix, path, content_type, cb, callback_arg);
    while (!ctx->is_last)
    {
        if ((status = process_blockwise_uploads(client, ctx)) != GOLIOTH_OK)
        {
            break;
        }
    }

    // Upload complete. Free the memory
    free(data_buff);
    free(ctx);

    return status;
}


/* Blockwise Downloads related functions */

// Function to initialize the blockwise_transfer structure for downloads
static void blockwise_download_init(struct blockwise_transfer *ctx,
                                    uint8_t *data_buf,
                                    const char *path_prefix,
                                    const char *path,
                                    write_block_cb cb,
                                    void *callback_arg)
{
    ctx->is_last = false;
    ctx->path_prefix = path_prefix;
    ctx->path = path;
    ctx->retry_count = 0;
    ctx->block_size = 0;
    ctx->offset = 0;
    ctx->block_buffer = data_buf;
    ctx->callback_arg = callback_arg;
    ctx->callback.write_cb = cb;
}

// Function to call the application's write block callback after a successful
// blockwise download
static enum golioth_status call_write_block_callback(struct blockwise_transfer *ctx)
{
    enum golioth_status status = ctx->callback.write_cb(ctx->offset,
                                                        ctx->block_buffer,
                                                        ctx->block_size,
                                                        ctx->is_last,
                                                        ctx->callback_arg);
    if (status == GOLIOTH_OK)
    {
        ctx->offset++;
    }
    return status;
}

// Blockwise download's internal callback function that the COAP client calls
static void on_block_rcvd(struct golioth_client *client,
                          const struct golioth_response *response,
                          const char *path,
                          const uint8_t *payload,
                          size_t payload_size,
                          bool is_last,
                          void *arg)
{
    // assert valid values of arg, payload size and block_buffer
    assert(arg);
    assert(payload_size <= CONFIG_GOLIOTH_BLOCKWISE_DOWNLOAD_BUFFER_SIZE);
    struct blockwise_transfer *ctx = (struct blockwise_transfer *) arg;
    assert(ctx->block_buffer);

    // copy blockwise download values returned by COAP client into ctx
    ctx->is_last = is_last;
    memcpy(ctx->block_buffer, payload, payload_size);
    ctx->block_size = payload_size;
    ctx->status = response->status;
}

// Function to download a single block
static void download_single_block(struct golioth_client *client, struct blockwise_transfer *ctx)
{
    enum golioth_status status = golioth_coap_client_get_block(client,
                                                               ctx->path_prefix,
                                                               ctx->path,
                                                               GOLIOTH_CONTENT_TYPE_JSON,
                                                               ctx->offset,
                                                               ctx->block_size,
                                                               on_block_rcvd,
                                                               ctx,
                                                               true,
                                                               GOLIOTH_SYS_WAIT_FOREVER);
    ctx->status = status;
}

// Function to handle blockwise download errors
// This function retries for all errors like invalid state, failed mem alloc,
// queue full, disconnects, etc returned by golioth_coap_client_get_block
static enum golioth_status handle_download_error(struct golioth_client *client,
                                                 struct blockwise_transfer *ctx)
{
    while (1)
    {
        GLTH_LOGE(TAG, "Blockwise download failed with error %d. Retrying.", ctx->status);
        golioth_sys_msleep(CONFIG_GOLIOTH_BLOCKWISE_RETRY_SLEEP_TIME_MS);
        download_single_block(client, ctx);
        if (ctx->status == GOLIOTH_OK)
        {
            ctx->retry_count = 0;
            ctx->status = call_write_block_callback(ctx);
            break;
        }
        else
        {
            if (CONFIG_GOLIOTH_BLOCKWISE_RETRY_COUNT != -1)
            {
                ctx->retry_count++;
                if (ctx->retry_count > CONFIG_GOLIOTH_BLOCKWISE_RETRY_COUNT)
                {
                    break;
                }
            }
        }
    }
    return ctx->status;
}

// Function to manage blockwise downloads and handle errors
static enum golioth_status process_blockwise_downloads(struct golioth_client *client,
                                                       struct blockwise_transfer *ctx)
{
    enum golioth_status status;
    download_single_block(client, ctx);
    if (ctx->status == GOLIOTH_OK)
    {
        status = call_write_block_callback(ctx);
    }
    else
    {
        status = handle_download_error(client, ctx);
    }
    return status;
}

enum golioth_status golioth_blockwise_get(struct golioth_client *client,
                                          const char *path_prefix,
                                          const char *path,
                                          write_block_cb cb,
                                          void *callback_arg)
{
    enum golioth_status status;

    if (!client || !path || !cb)
    {
        return GOLIOTH_ERR_NULL;
    }

    if (!path_prefix)
    {
        path_prefix = "";
    }

    struct blockwise_transfer *ctx = malloc(sizeof(struct blockwise_transfer));
    if (!ctx)
    {
        return GOLIOTH_ERR_MEM_ALLOC;
    }

    uint8_t *data_buff = malloc(CONFIG_GOLIOTH_BLOCKWISE_DOWNLOAD_BUFFER_SIZE);
    if (!data_buff)
    {
        free(ctx);
        return GOLIOTH_ERR_MEM_ALLOC;
    }

    blockwise_download_init(ctx, data_buff, path_prefix, path, cb, callback_arg);
    while (!ctx->is_last)
    {
        if ((status = process_blockwise_downloads(client, ctx)) != GOLIOTH_OK)
        {
            break;
        }
    }

    // Download complete. Free the memory
    free(data_buff);
    free(ctx);

    return status;
}
