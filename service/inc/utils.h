/*
** Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

/*
** Changes from Qualcomm Technologies, Inc. are provided under the following license:
** Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
** SPDX-License-Identifier: BSD-3-Clause
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted (subject to the limitations in the
** disclaimer below) provided that the following conditions are met:
**
**    * Redistributions of source code must retain the above copyright
**      notice, this list of conditions and the following disclaimer.
**
**    * Redistributions in binary form must reproduce the above
**      copyright notice, this list of conditions and the following
**      disclaimer in the documentation and/or other materials provided
**      with the distribution.
**
**    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
**      contributors may be used to endorse or promote products derived
**      from this software without specific prior written permission.
**
** NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
** GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
** HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
** GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
** IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#ifndef __UTILS_H__
#define __UTILS_H__
#include "ar_osal_error.h"
#include <log/log.h>

#ifdef USE_CLOG
#include <clog.h>
#include <clog_ids.h>

#define AGM_CLOG_CONTEXT_ID CLOG_LCTX_SERV
#define AGM_CLOG_APP_NAME "AGM"
#define AGM_CLOG_CONTEXT_NAME "SERV"
#define AGM_CLOG_CONTEXT_DESC "AGM Service Core"

static inline clog_context_t agm_get_clog_context(void)
{
    static clog_context_t agm_clog_ctx = NULL;
    static int agm_clog_init_done = 0;

    if (!agm_clog_init_done) {
        if (CLOG_IS_INITIALIZED() != CLOG_RETURN_TRUE)
            CLOG_REGISTER_LIBRARY();

        if (CLOG_IS_INITIALIZED() == CLOG_RETURN_TRUE) {
            agm_clog_ctx = CLOG_REGISTER_CONTEXT(AGM_CLOG_CONTEXT_ID,
                                                 AGM_CLOG_CONTEXT_NAME,
                                                 AGM_CLOG_CONTEXT_DESC,
                                                 CLOG_DIAG_SSID_NOT_AVAILABLE,
                                                 AGM_CLOG_APP_NAME,
                                                 AGM_CLOG_APP_NAME);
        }
        agm_clog_init_done = 1;
    }

    return agm_clog_ctx;
}

#define AGM_CLOG_WRAPPER(clog_fn, alog_fn, arg, ...) do { \
    clog_context_t _ctx = agm_get_clog_context(); \
    if (_ctx) \
        clog_fn(_ctx, "%s: %d " arg, __func__, __LINE__, ##__VA_ARGS__); \
    else \
        alog_fn("%s: %d " arg, __func__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define AGM_LOGE(arg,...) AGM_CLOG_WRAPPER(CLOGE, ALOGE, arg, ##__VA_ARGS__)
#define AGM_LOGD(arg,...) AGM_CLOG_WRAPPER(CLOGD, ALOGD, arg, ##__VA_ARGS__)
#define AGM_LOGI(arg,...) AGM_CLOG_WRAPPER(CLOGI, ALOGI, arg, ##__VA_ARGS__)
#define AGM_LOGV(arg,...) AGM_CLOG_WRAPPER(CLOGV, ALOGV, arg, ##__VA_ARGS__)

#elif defined(USE_DLT)
#include <dlt/dlt.h>
#include <stdio.h>

// Declare the DLT context
extern DLT_DECLARE_CONTEXT(agm_dlt_ctx);

// DLT Wrapper: Formats the string before sending to DLT
#define AGM_DLT_WRAPPER(level, fmt, ...) do { \
    char _buf[256]; \
    snprintf(_buf, sizeof(_buf), "%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__); \
    DLT_LOG(agm_dlt_ctx, level, DLT_STRING(_buf)); \
} while(0)

#define AGM_LOGE(arg,...) AGM_DLT_WRAPPER(DLT_LOG_ERROR, arg, ##__VA_ARGS__)
#define AGM_LOGD(arg,...) AGM_DLT_WRAPPER(DLT_LOG_DEBUG, arg, ##__VA_ARGS__)
#define AGM_LOGI(arg,...) AGM_DLT_WRAPPER(DLT_LOG_INFO,  arg, ##__VA_ARGS__)
#define AGM_LOGV(arg,...) AGM_DLT_WRAPPER(DLT_LOG_VERBOSE, arg, ##__VA_ARGS__)

#else
#define AGM_LOGE(arg,...) ALOGE("%s: %d "  arg, __func__, __LINE__, ##__VA_ARGS__)
#define AGM_LOGD(arg,...) ALOGD("%s: %d "  arg, __func__, __LINE__, ##__VA_ARGS__)
#define AGM_LOGI(arg,...) ALOGI("%s: %d "  arg, __func__, __LINE__, ##__VA_ARGS__)
#define AGM_LOGV(arg,...) ALOGV("%s: %d "  arg, __func__, __LINE__, ##__VA_ARGS__)
#endif

/*convert osal error codes to lnx error codes*/
int ar_err_get_lnx_err_code(uint32_t error);
/*helper to print errors in string form*/
char *ar_err_get_err_str(uint32_t error);

#endif /*__UTILS_H*/
