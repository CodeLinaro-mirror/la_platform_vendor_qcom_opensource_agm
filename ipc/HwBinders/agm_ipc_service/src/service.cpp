/*
 * Copyright (c) 2019 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "vendor.qti.hardware.AGMIPC@1.0-service"
#include <vendor/qti/hardware/AGMIPC/1.0/IAGM.h>
#include <hidl/LegacySupport.h>
#include "inc/agm_server_wrapper.h"
#if defined(AR_EARLY_CHIME) || defined(PLATFORM_MSMNILE_AU)
#include <android-base/properties.h>
#endif
#include <system/thread_defs.h>
#include <selinux/android.h>

#ifdef AR_EARLY_CHIME
#include <agm_conn_server.h>
#include <chrono>
#include <errno.h>
#endif

using vendor::qti::hardware::AGMIPC::V1_0::IAGM;
using vendor::qti::hardware::AGMIPC::V1_0::implementation::AGM;
using android::hardware::defaultPassthroughServiceImplementation;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;

#ifdef PLATFORM_MSMNILE_AU
static void place_marker(char const *name)
{
   int fd=open("/sys/kernel/boot_kpi/kpi_values", O_WRONLY);
   if (fd > 0)
   {
       /* Only allow marker text shorter than MARKER_STRING_WIDTH */
       char earlyapp[100] = {0};
       strlcpy(earlyapp, name, sizeof(earlyapp));
       write(fd, earlyapp, strlen(earlyapp));
       close(fd);
   }
}
#endif

static inline bool checkBinderServiceReady() {
#ifdef AR_EARLY_CHIME
    static bool flag = false;
    if (flag) {
        /* return true if servicemanager online once */
        return true;
    } else {
        flag = android::base::WaitForProperty("hwservicemanager.ready", "true", std::chrono::milliseconds(1000));
    }
    return flag;
#else
    return true;
#endif
}

int main(int argc, char *argv[]) {
#ifdef PLATFORM_MSMNILE_AU
    place_marker("M - AGM Service Starting...");
#endif
    sp<IAGM> service = new AGM();
    int context_initialized = -1;
#ifdef AR_EARLY_CHIME
    init_service_socket();
#endif
    AGM *temp = static_cast<AGM *>(service.get());
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_URGENT_AUDIO);
    if (temp->is_agm_initialized()) {
        if (argc > 1) {
            FILE *fptr;
            int num = 1;

            // use appropriate location if you are using MacOS or Linux
            fptr = fopen("/vendor_early_services/agm.txt","w");

            if (fptr != NULL)
            {
                fprintf(fptr,"%d",num);
                fclose(fptr);
            }

            do {
                if (context_initialized == -1)
                    context_initialized = selinux_android_setcon("u:r:vendor_agmservice_qti:s0");

                if (context_initialized != -1)
                    break;
                else
                    sleep(1);
            } while(1);
#ifndef AR_EARLY_CHIME
            while (!checkBinderServiceReady())
                sleep(1);
#endif
        }
        configureRpcThreadpool(16, true /*callerWillJoin*/);
#ifndef AR_EARLY_CHIME
        if (android::OK !=  service->registerAsService()) {
            ALOGE("%s:AGM service cannot be registered!", __func__);
            return 1;
        }
#endif
#ifdef PLATFORM_MSMNILE_AU
        place_marker("M - AGM Service Created...");
        android::base::SetProperty("vendor.audio.feature.agm.enable", "running");
#endif
        ALOGE("%s:AGM service is registered!", __func__);
#ifdef AR_EARLY_CHIME
        //TODO delay
        sleep(1);
#endif
        joinRpcThreadpool();
    }
    return 1;
};
