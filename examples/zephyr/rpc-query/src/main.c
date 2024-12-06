/*
 * Copyright (c) 2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rpc_sample, LOG_LEVEL_DBG);

#include <golioth/client.h>
#include <golioth/rpc.h>
#include <samples/common/sample_credentials.h>
#include <samples/common/net_connect.h>
#include <string.h>
#include <zephyr/kernel.h>

static K_SEM_DEFINE(connected, 0, 1);

static enum golioth_rpc_status on_multiply(zcbor_state_t *request_params_array,
                                           zcbor_state_t *response_detail_map,
                                           void *callback_arg)
{
    double a, b;
    double value;
    bool ok;

    ok = zcbor_float_decode(request_params_array, &a)
        && zcbor_float_decode(request_params_array, &b);
    if (!ok)
    {
        LOG_ERR("Failed to decode array items");
        return GOLIOTH_RPC_INVALID_ARGUMENT;
    }

    value = a * b;

    LOG_DBG("%lf * %lf = %lf", a, b, value);

    ok = zcbor_tstr_put_lit(response_detail_map, "value")
        && zcbor_float64_put(response_detail_map, value);
    if (!ok)
    {
        LOG_ERR("Failed to encode value");
        return GOLIOTH_RPC_RESOURCE_EXHAUSTED;
    }

    return GOLIOTH_RPC_OK;
}

static enum golioth_rpc_status on_query(zcbor_state_t *request_params_array,
                                        zcbor_state_t *response_detail_map,
                                        void *callback_arg)
{
    bool ok = zcbor_tstr_put_lit(response_detail_map, "multiply")
        && zcbor_float64_put(response_detail_map, 0x321)
        && zcbor_tstr_put_lit(response_detail_map, "reboot")
        && zcbor_float64_put(response_detail_map, 0x0);

    if (!ok)
    {
        LOG_ERR("Failed to encode value");
        return GOLIOTH_RPC_RESOURCE_EXHAUSTED;
    }

    return GOLIOTH_RPC_OK;
}

static void on_client_event(struct golioth_client *client,
                            enum golioth_client_event event,
                            void *arg)
{
    bool is_connected = (event == GOLIOTH_CLIENT_EVENT_CONNECTED);
    if (is_connected)
    {
        k_sem_give(&connected);
    }
    LOG_INF("Golioth client %s", is_connected ? "connected" : "disconnected");
}

int main(void)
{
    LOG_DBG("Start RPC sample");

    net_connect();

    /* Note: In production, you would provision unique credentials onto each
     * device. For simplicity, we provide a utility to hardcode credentials as
     * kconfig options in the samples.
     */
    const struct golioth_client_config *client_config = golioth_sample_credentials_get();

    struct golioth_client *client = golioth_client_create(client_config);
    struct golioth_rpc *rpc = golioth_rpc_init(client);

    golioth_client_register_event_callback(client, on_client_event, NULL);

    k_sem_take(&connected, K_FOREVER);


    // This is required for the demo, but might not be required long term and the callback 
    // is captured by the SDK
    golioth_rpc_register(rpc, "query", on_query, NULL);


    // Register the rpc itself and the parameters individually
    int err = golioth_rpc_register(rpc, "multiply", on_multiply, NULL);
    golioth_rpc_add_param_info(rpc,"multiply", "int_a",GOLIOTH_RPC_PARAM_TYPE_NUM);
    golioth_rpc_add_param_info(rpc,"multiply", "int_b",GOLIOTH_RPC_PARAM_TYPE_NUM);

    if (err)
    {
        LOG_ERR("Failed to register RPC: %d", err);
    }

    while (true)
    {
        k_sleep(K_SECONDS(5));
    }

    return 0;
}
