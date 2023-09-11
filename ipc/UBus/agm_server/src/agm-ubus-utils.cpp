/**
* * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* * SPDX-License-Identifier: BSD-3-Clause-Clear
* *
**/

#define LOG_TAG "agm_ubus_utils"
#include <stdio.h>
#include <errno.h>
#include <malloc.h>

#include "inc/agm-ubus-utils.h"
#include "utils.h"

struct ubus_context *ctx = NULL;

struct ubus_context *agm_ubus_new_connection(const char *ubus_socket){
    uloop_init_t uloop_init_ptr = NULL;

    AGM_LOGE("%s:Enter\n", __func__);

    if (!ctx) {
        uloop_init_ptr();
        signal(SIGPIPE, SIG_IGN);

        ctx = ubus_connect(ubus_socket);

        if (!ctx) {
            AGM_LOGE("Failed to connect to ubus\n");
            return NULL;
        }
        AGM_LOGE("connect to ubus success\n");

        ubus_add_uloop(ctx);
    }
    AGM_LOGE("%s:Exit\n", __func__);
    return ctx;
}