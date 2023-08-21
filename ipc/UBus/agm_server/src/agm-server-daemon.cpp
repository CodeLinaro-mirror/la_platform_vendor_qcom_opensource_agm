/**
* * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* * SPDX-License-Identifier: BSD-3-Clause-Clear
* *
**/

#define LOG_TAG "agm_server_daemon"
#include <glib.h>
#include <signal.h>
#include <stdio.h>
#include "inc/agm_server_wrapper_ubus.h"
#include <utils.h>


void signal_handler(int sig) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:
        case SIGABRT:
        case SIGQUIT:
        case SIGKILL:
        default:
            AGM_LOGE("Terminating signal received\n");
            ipc_agm_deinit();
            break;
    }
}

int main() {
    int rc = 0;

    rc = ipc_agm_init();
    if (rc != 0) {
        AGM_LOGE("AGM init failed\n");
        return rc;
    }

    AGM_LOGD("agm init done\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGABRT, signal_handler);

    return 0;
}
