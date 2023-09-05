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
#ifdef PLATFORM_MSMNILE_AU
#include <cutils/properties.h>
#endif
#include <system/thread_defs.h>

#ifdef AR_EARLY_CHIME
#include <agm_conn_server.h>
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

int main() {
#ifdef PLATFORM_MSMNILE_AU
    place_marker("M - AGM Service Starting...");
#endif
    sp<IAGM> service = new AGM();
#ifdef AR_EARLY_CHIME
    init_service_socket();
#endif
    AGM *temp = static_cast<AGM *>(service.get());
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_URGENT_AUDIO);
    if (temp->is_agm_initialized()) {
        configureRpcThreadpool(16, true /*callerWillJoin*/);
        if(android::OK !=  service->registerAsService()) {
            ALOGE("%s:AGM service cannot be registered!", __func__);
           return 1;
        }
#ifdef PLATFORM_MSMNILE_AU
        property_set("vendor.audio.feature.agm.enable", "running");
        place_marker("M - AGM Service Created...");
#endif
#ifdef AR_EARLY_CHIME
        deinit_service_socket();
#endif
        joinRpcThreadpool();
    }
    return 1;
};
