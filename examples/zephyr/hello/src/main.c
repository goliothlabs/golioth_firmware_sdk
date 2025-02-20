/*
 * Copyright (c) 2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hello_zephyr, LOG_LEVEL_DBG);

#include <golioth/client.h>
#include <samples/common/sample_credentials.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <samples/common/net_connect.h>
#include <golioth/stream.h>
#include <zcbor_encode.h>

struct golioth_client *client;
static K_SEM_DEFINE(connected, 0, 1);

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

unsigned char test_data_json[] = {
    0x7b, 0x0a, 0x20, 0x20, 0x22, 0x30, 0x22, 0x3a, 0x20, 0x22, 0x7a, 0x65, 0x72, 0x6f, 0x22, 0x2c,
    0x0a, 0x20, 0x20, 0x22, 0x31, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20,
    0x20, 0x22, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x77, 0x6f, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x33, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x34, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x6f, 0x75, 0x72, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x35,
    0x22, 0x3a, 0x20, 0x22, 0x66, 0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x36, 0x22,
    0x3a, 0x20, 0x22, 0x73, 0x69, 0x78, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x37, 0x22, 0x3a, 0x20,
    0x22, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x38, 0x22, 0x3a, 0x20,
    0x22, 0x65, 0x69, 0x67, 0x68, 0x74, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x39, 0x22, 0x3a, 0x20,
    0x22, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x22, 0x3a, 0x20,
    0x22, 0x74, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x31, 0x22, 0x3a, 0x20, 0x22,
    0x65, 0x6c, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x32, 0x22, 0x3a,
    0x20, 0x22, 0x74, 0x77, 0x65, 0x6c, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x33,
    0x22, 0x3a, 0x20, 0x22, 0x74, 0x68, 0x69, 0x72, 0x74, 0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20,
    0x20, 0x22, 0x31, 0x34, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x6f, 0x75, 0x72, 0x74, 0x65, 0x65, 0x6e,
    0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x69, 0x66, 0x74,
    0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x36, 0x22, 0x3a, 0x20, 0x22, 0x73,
    0x69, 0x78, 0x74, 0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x37, 0x22, 0x3a,
    0x20, 0x22, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x31, 0x38, 0x22, 0x3a, 0x20, 0x22, 0x65, 0x69, 0x67, 0x68, 0x74, 0x65, 0x65, 0x6e, 0x22,
    0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x39, 0x22, 0x3a, 0x20, 0x22, 0x6e, 0x69, 0x6e, 0x65, 0x74,
    0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x32, 0x30, 0x22, 0x3a, 0x20, 0x22, 0x74,
    0x77, 0x65, 0x6e, 0x74, 0x79, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x32, 0x31, 0x22, 0x3a, 0x20,
    0x22, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x32, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x74, 0x77,
    0x6f, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x32, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x77, 0x65,
    0x6e, 0x74, 0x79, 0x2d, 0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x32,
    0x34, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x66, 0x6f, 0x75, 0x72,
    0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x32, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x77, 0x65, 0x6e,
    0x74, 0x79, 0x2d, 0x66, 0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x32, 0x36, 0x22,
    0x3a, 0x20, 0x22, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x73, 0x69, 0x78, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x32, 0x37, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d,
    0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x32, 0x38, 0x22, 0x3a, 0x20,
    0x22, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x65, 0x69, 0x67, 0x68, 0x74, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x32, 0x39, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d,
    0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x33, 0x30, 0x22, 0x3a, 0x20, 0x22,
    0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x33, 0x31, 0x22, 0x3a,
    0x20, 0x22, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20,
    0x20, 0x22, 0x33, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d, 0x74,
    0x77, 0x6f, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x33, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x68,
    0x69, 0x72, 0x74, 0x79, 0x2d, 0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x33, 0x34, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d, 0x66, 0x6f, 0x75,
    0x72, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x33, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x68, 0x69,
    0x72, 0x74, 0x79, 0x2d, 0x66, 0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x33, 0x36,
    0x22, 0x3a, 0x20, 0x22, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d, 0x73, 0x69, 0x78, 0x22, 0x2c,
    0x0a, 0x20, 0x20, 0x22, 0x33, 0x37, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79,
    0x2d, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x33, 0x38, 0x22, 0x3a,
    0x20, 0x22, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d, 0x65, 0x69, 0x67, 0x68, 0x74, 0x22, 0x2c,
    0x0a, 0x20, 0x20, 0x22, 0x33, 0x39, 0x22, 0x3a, 0x20, 0x22, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79,
    0x2d, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x34, 0x30, 0x22, 0x3a, 0x20,
    0x22, 0x66, 0x6f, 0x72, 0x74, 0x79, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x34, 0x31, 0x22, 0x3a,
    0x20, 0x22, 0x66, 0x6f, 0x72, 0x74, 0x79, 0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x34, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x6f, 0x72, 0x74, 0x79, 0x2d, 0x74, 0x77, 0x6f,
    0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x34, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x6f, 0x72, 0x74,
    0x79, 0x2d, 0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x34, 0x34, 0x22,
    0x3a, 0x20, 0x22, 0x66, 0x6f, 0x72, 0x74, 0x79, 0x2d, 0x66, 0x6f, 0x75, 0x72, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x34, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x6f, 0x72, 0x74, 0x79, 0x2d, 0x66,
    0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x34, 0x36, 0x22, 0x3a, 0x20, 0x22, 0x66,
    0x6f, 0x72, 0x74, 0x79, 0x2d, 0x73, 0x69, 0x78, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x34, 0x37,
    0x22, 0x3a, 0x20, 0x22, 0x66, 0x6f, 0x72, 0x74, 0x79, 0x2d, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x22,
    0x2c, 0x0a, 0x20, 0x20, 0x22, 0x34, 0x38, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x6f, 0x72, 0x74, 0x79,
    0x2d, 0x65, 0x69, 0x67, 0x68, 0x74, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x34, 0x39, 0x22, 0x3a,
    0x20, 0x22, 0x66, 0x6f, 0x72, 0x74, 0x79, 0x2d, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20,
    0x20, 0x22, 0x35, 0x30, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x69, 0x66, 0x74, 0x79, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x35, 0x31, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x69, 0x66, 0x74, 0x79, 0x2d, 0x6f,
    0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x35, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x69,
    0x66, 0x74, 0x79, 0x2d, 0x74, 0x77, 0x6f, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x35, 0x33, 0x22,
    0x3a, 0x20, 0x22, 0x66, 0x69, 0x66, 0x74, 0x79, 0x2d, 0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c,
    0x0a, 0x20, 0x20, 0x22, 0x35, 0x34, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x69, 0x66, 0x74, 0x79, 0x2d,
    0x66, 0x6f, 0x75, 0x72, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x35, 0x35, 0x22, 0x3a, 0x20, 0x22,
    0x66, 0x69, 0x66, 0x74, 0x79, 0x2d, 0x66, 0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x35, 0x36, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x69, 0x66, 0x74, 0x79, 0x2d, 0x73, 0x69, 0x78, 0x22,
    0x2c, 0x0a, 0x20, 0x20, 0x22, 0x35, 0x37, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x69, 0x66, 0x74, 0x79,
    0x2d, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x35, 0x38, 0x22, 0x3a,
    0x20, 0x22, 0x66, 0x69, 0x66, 0x74, 0x79, 0x2d, 0x65, 0x69, 0x67, 0x68, 0x74, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x35, 0x39, 0x22, 0x3a, 0x20, 0x22, 0x66, 0x69, 0x66, 0x74, 0x79, 0x2d, 0x6e,
    0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x36, 0x30, 0x22, 0x3a, 0x20, 0x22, 0x73,
    0x69, 0x78, 0x74, 0x79, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x36, 0x31, 0x22, 0x3a, 0x20, 0x22,
    0x73, 0x69, 0x78, 0x74, 0x79, 0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x36,
    0x32, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x69, 0x78, 0x74, 0x79, 0x2d, 0x74, 0x77, 0x6f, 0x22, 0x2c,
    0x0a, 0x20, 0x20, 0x22, 0x36, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x69, 0x78, 0x74, 0x79, 0x2d,
    0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x36, 0x34, 0x22, 0x3a, 0x20,
    0x22, 0x73, 0x69, 0x78, 0x74, 0x79, 0x2d, 0x66, 0x6f, 0x75, 0x72, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x36, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x69, 0x78, 0x74, 0x79, 0x2d, 0x66, 0x69, 0x76,
    0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x36, 0x36, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x69, 0x78,
    0x74, 0x79, 0x2d, 0x73, 0x69, 0x78, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x36, 0x37, 0x22, 0x3a,
    0x20, 0x22, 0x73, 0x69, 0x78, 0x74, 0x79, 0x2d, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x36, 0x38, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x69, 0x78, 0x74, 0x79, 0x2d, 0x65,
    0x69, 0x67, 0x68, 0x74, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x36, 0x39, 0x22, 0x3a, 0x20, 0x22,
    0x73, 0x69, 0x78, 0x74, 0x79, 0x2d, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x37, 0x30, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x37, 0x31, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79,
    0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x37, 0x32, 0x22, 0x3a, 0x20, 0x22,
    0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x74, 0x77, 0x6f, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x37, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x74,
    0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x37, 0x34, 0x22, 0x3a, 0x20, 0x22,
    0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x66, 0x6f, 0x75, 0x72, 0x22, 0x2c, 0x0a, 0x20,
    0x20, 0x22, 0x37, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79, 0x2d,
    0x66, 0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x37, 0x36, 0x22, 0x3a, 0x20, 0x22,
    0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x73, 0x69, 0x78, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x37, 0x37, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x73,
    0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x37, 0x38, 0x22, 0x3a, 0x20, 0x22,
    0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x65, 0x69, 0x67, 0x68, 0x74, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x37, 0x39, 0x22, 0x3a, 0x20, 0x22, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x74, 0x79,
    0x2d, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x38, 0x30, 0x22, 0x3a, 0x20,
    0x22, 0x65, 0x69, 0x67, 0x68, 0x74, 0x79, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x38, 0x31, 0x22,
    0x3a, 0x20, 0x22, 0x65, 0x69, 0x67, 0x68, 0x74, 0x79, 0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x38, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x65, 0x69, 0x67, 0x68, 0x74, 0x79, 0x2d,
    0x74, 0x77, 0x6f, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x38, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x65,
    0x69, 0x67, 0x68, 0x74, 0x79, 0x2d, 0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x38, 0x34, 0x22, 0x3a, 0x20, 0x22, 0x65, 0x69, 0x67, 0x68, 0x74, 0x79, 0x2d, 0x66, 0x6f,
    0x75, 0x72, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x38, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x65, 0x69,
    0x67, 0x68, 0x74, 0x79, 0x2d, 0x66, 0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x38,
    0x36, 0x22, 0x3a, 0x20, 0x22, 0x65, 0x69, 0x67, 0x68, 0x74, 0x79, 0x2d, 0x73, 0x69, 0x78, 0x22,
    0x2c, 0x0a, 0x20, 0x20, 0x22, 0x38, 0x37, 0x22, 0x3a, 0x20, 0x22, 0x65, 0x69, 0x67, 0x68, 0x74,
    0x79, 0x2d, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x38, 0x38, 0x22,
    0x3a, 0x20, 0x22, 0x65, 0x69, 0x67, 0x68, 0x74, 0x79, 0x2d, 0x65, 0x69, 0x67, 0x68, 0x74, 0x22,
    0x2c, 0x0a, 0x20, 0x20, 0x22, 0x38, 0x39, 0x22, 0x3a, 0x20, 0x22, 0x65, 0x69, 0x67, 0x68, 0x74,
    0x79, 0x2d, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x39, 0x30, 0x22, 0x3a,
    0x20, 0x22, 0x6e, 0x69, 0x6e, 0x65, 0x74, 0x79, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x39, 0x31,
    0x22, 0x3a, 0x20, 0x22, 0x6e, 0x69, 0x6e, 0x65, 0x74, 0x79, 0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x2c,
    0x0a, 0x20, 0x20, 0x22, 0x39, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x6e, 0x69, 0x6e, 0x65, 0x74, 0x79,
    0x2d, 0x74, 0x77, 0x6f, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x39, 0x33, 0x22, 0x3a, 0x20, 0x22,
    0x6e, 0x69, 0x6e, 0x65, 0x74, 0x79, 0x2d, 0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20,
    0x20, 0x22, 0x39, 0x34, 0x22, 0x3a, 0x20, 0x22, 0x6e, 0x69, 0x6e, 0x65, 0x74, 0x79, 0x2d, 0x66,
    0x6f, 0x75, 0x72, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x39, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x6e,
    0x69, 0x6e, 0x65, 0x74, 0x79, 0x2d, 0x66, 0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x39, 0x36, 0x22, 0x3a, 0x20, 0x22, 0x6e, 0x69, 0x6e, 0x65, 0x74, 0x79, 0x2d, 0x73, 0x69, 0x78,
    0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x39, 0x37, 0x22, 0x3a, 0x20, 0x22, 0x6e, 0x69, 0x6e, 0x65,
    0x74, 0x79, 0x2d, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x39, 0x38,
    0x22, 0x3a, 0x20, 0x22, 0x6e, 0x69, 0x6e, 0x65, 0x74, 0x79, 0x2d, 0x65, 0x69, 0x67, 0x68, 0x74,
    0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x39, 0x39, 0x22, 0x3a, 0x20, 0x22, 0x6e, 0x69, 0x6e, 0x65,
    0x74, 0x79, 0x2d, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x30,
    0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x22,
    0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x31, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20,
    0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x6f, 0x6e, 0x65, 0x22,
    0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20,
    0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x77, 0x6f, 0x22,
    0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20,
    0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x72, 0x65,
    0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x34, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e,
    0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x66, 0x6f,
    0x75, 0x72, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x6f,
    0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x66,
    0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x36, 0x22, 0x3a, 0x20, 0x22,
    0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20,
    0x73, 0x69, 0x78, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x37, 0x22, 0x3a, 0x20, 0x22,
    0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20,
    0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x38, 0x22, 0x3a,
    0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e,
    0x64, 0x20, 0x65, 0x69, 0x67, 0x68, 0x74, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x30, 0x39,
    0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20,
    0x61, 0x6e, 0x64, 0x20, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x31,
    0x30, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64,
    0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x31,
    0x31, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64,
    0x20, 0x61, 0x6e, 0x64, 0x20, 0x65, 0x6c, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x31, 0x31, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64,
    0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x77, 0x65, 0x6c, 0x76, 0x65, 0x22, 0x2c,
    0x0a, 0x20, 0x20, 0x22, 0x31, 0x31, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68,
    0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74,
    0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x31, 0x34, 0x22, 0x3a, 0x20, 0x22,
    0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20,
    0x66, 0x6f, 0x75, 0x72, 0x74, 0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x31,
    0x35, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64,
    0x20, 0x61, 0x6e, 0x64, 0x20, 0x66, 0x69, 0x66, 0x74, 0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20,
    0x20, 0x22, 0x31, 0x31, 0x36, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e,
    0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x73, 0x69, 0x78, 0x74, 0x65, 0x65, 0x6e,
    0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x31, 0x37, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65,
    0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x73, 0x65, 0x76,
    0x65, 0x6e, 0x74, 0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x31, 0x38, 0x22,
    0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61,
    0x6e, 0x64, 0x20, 0x65, 0x69, 0x67, 0x68, 0x74, 0x65, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x31, 0x31, 0x39, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64,
    0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x6e, 0x69, 0x6e, 0x65, 0x74, 0x65, 0x65, 0x6e,
    0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x32, 0x30, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65,
    0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x77, 0x65,
    0x6e, 0x74, 0x79, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x32, 0x31, 0x22, 0x3a, 0x20, 0x22,
    0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20,
    0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x31, 0x32, 0x32, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72,
    0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x74, 0x77,
    0x6f, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x32, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e,
    0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x77,
    0x65, 0x6e, 0x74, 0x79, 0x2d, 0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x31, 0x32, 0x34, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72,
    0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x66, 0x6f,
    0x75, 0x72, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x32, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x6f,
    0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74,
    0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x66, 0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x31, 0x32, 0x36, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72,
    0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x73, 0x69,
    0x78, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x32, 0x37, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e,
    0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x77,
    0x65, 0x6e, 0x74, 0x79, 0x2d, 0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22,
    0x31, 0x32, 0x38, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72,
    0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x65, 0x69,
    0x67, 0x68, 0x74, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x32, 0x39, 0x22, 0x3a, 0x20, 0x22,
    0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20,
    0x74, 0x77, 0x65, 0x6e, 0x74, 0x79, 0x2d, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20,
    0x22, 0x31, 0x33, 0x30, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64,
    0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x22, 0x2c,
    0x0a, 0x20, 0x20, 0x22, 0x31, 0x33, 0x31, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68,
    0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74,
    0x79, 0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x33, 0x32, 0x22, 0x3a,
    0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e,
    0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d, 0x74, 0x77, 0x6f, 0x22, 0x2c, 0x0a, 0x20,
    0x20, 0x22, 0x31, 0x33, 0x33, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e,
    0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d,
    0x74, 0x68, 0x72, 0x65, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x33, 0x34, 0x22, 0x3a,
    0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e,
    0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d, 0x66, 0x6f, 0x75, 0x72, 0x22, 0x2c, 0x0a,
    0x20, 0x20, 0x22, 0x31, 0x33, 0x35, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75,
    0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79,
    0x2d, 0x66, 0x69, 0x76, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x33, 0x36, 0x22, 0x3a,
    0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e,
    0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d, 0x73, 0x69, 0x78, 0x22, 0x2c, 0x0a, 0x20,
    0x20, 0x22, 0x31, 0x33, 0x37, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e,
    0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d,
    0x73, 0x65, 0x76, 0x65, 0x6e, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x33, 0x38, 0x22, 0x3a,
    0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e,
    0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74, 0x79, 0x2d, 0x65, 0x69, 0x67, 0x68, 0x74, 0x22, 0x2c,
    0x0a, 0x20, 0x20, 0x22, 0x31, 0x33, 0x39, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68,
    0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x74, 0x68, 0x69, 0x72, 0x74,
    0x79, 0x2d, 0x6e, 0x69, 0x6e, 0x65, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x34, 0x30, 0x22,
    0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64, 0x20, 0x61,
    0x6e, 0x64, 0x20, 0x66, 0x6f, 0x72, 0x74, 0x79, 0x22, 0x2c, 0x0a, 0x20, 0x20, 0x22, 0x31, 0x34,
    0x31, 0x22, 0x3a, 0x20, 0x22, 0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64,
    0x20, 0x61, 0x6e, 0x64, 0x20, 0x66, 0x6f, 0x72, 0x74, 0x79, 0x2d, 0x6f, 0x6e, 0x65, 0x22, 0x0a,
    0x7d, 0x0a};

unsigned int test_data_json_len = 3666;

struct block_upload_source
{
    uint8_t *buf;
    size_t len;
};

static enum golioth_status block_upload_read_chunk(uint32_t block_idx,
                                                   uint8_t *block_buffer,
                                                   size_t *block_size,
                                                   bool *is_last,
                                                   void *arg)
{
    size_t bu_max_block_size = *block_size;
    const struct block_upload_source *bu_source = arg;
    size_t bu_offset = block_idx * bu_max_block_size;
    size_t bu_size = bu_source->len - bu_offset;

    if (bu_offset >= bu_source->len)
    {
        GLTH_LOGE(TAG, "Calculated offset is past end of buffer: %zu", bu_offset);
        goto bu_error;
    }

    if (bu_size <= bu_max_block_size)
    {
        *block_size = bu_size;
        *is_last = true;
    }

    GLTH_LOGI(TAG,
              "block-idx: %" PRIu32 " bu_offset: %zu bytes_remaining: %zu",
              block_idx,
              bu_offset,
              bu_size);

    memcpy(block_buffer, bu_source->buf + bu_offset, *block_size);
    return GOLIOTH_OK;

bu_error:
    *block_size = 0;
    *is_last = true;

    return GOLIOTH_ERR_NO_MORE_DATA;
}


void test_block_upload(struct golioth_client *client)
{
    struct block_upload_source bu_source = {.buf = (uint8_t *) test_data_json,
                                            .len = test_data_json_len};


    int err = golioth_stream_set_blockwise_sync(client, "block_upload", GOLIOTH_CONTENT_TYPE_JSON,
                                                block_upload_read_chunk, (void *) &bu_source);

    if (err) {
        GLTH_LOGE(TAG, "Stream error: %d", err);
    } else {
        GLTH_LOGI(TAG, "Block upload successful!");
    }

}
int main(void)
{
    int counter = 0;

    LOG_DBG("start hello sample");

    net_connect();

    /* Note: In production, you would provision unique credentials onto each
     * device. For simplicity, we provide a utility to hardcode credentials as
     * kconfig options in the samples.
     */
    const struct golioth_client_config *client_config = golioth_sample_credentials_get();

    client = golioth_client_create(client_config);

    golioth_client_register_event_callback(client, on_client_event, NULL);

    k_sem_take(&connected, K_FOREVER);

    while (true)
    {
        LOG_INF("Sending hello! %d", counter);

        ++counter;
        k_sleep(K_SECONDS(15));

        test_block_upload(client);

    }

    return 0;
}
