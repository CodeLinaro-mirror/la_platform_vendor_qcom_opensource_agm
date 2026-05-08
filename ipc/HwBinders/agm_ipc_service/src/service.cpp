/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 */

#define LOG_TAG "vendor.qti.hardware.AGMIPC@1.0-service"
#include <vendor/qti/hardware/AGMIPC/1.0/IAGM.h>
#include <hidl/LegacySupport.h>
#include "inc/agm_server_wrapper.h"
#include <android-base/properties.h>
#include <agm/utils.h>
#include <unistd.h>

using vendor::qti::hardware::AGMIPC::V1_0::IAGM;
using vendor::qti::hardware::AGMIPC::V1_0::implementation::AGM;
using android::hardware::defaultPassthroughServiceImplementation;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;

static inline bool checkBinderServiceReady() {
#ifdef AR_EARLY_AUDIO
    static bool flag = false;
    if (flag) {
        /* return true if servicemanager online once */
        return true;
    } else {
        flag = android::base::WaitForProperty("hwservicemanager.ready", "true", std::chrono::milliseconds(1000));
    }
    if (flag) {
        AGM_LOGI("hwservicemanager service is ready");
    } else {
        AGM_LOGI("hwservicemanager service is not ready");
    }
    return flag;
#else
    return true;
#endif
}

int main() {
#ifdef AR_EARLY_AUDIO
    freopen("/dev/kmsg", "a", stdout);
    freopen("/dev/kmsg", "a", stderr);
#endif
    AGM_LOGI("AGM service startup");
    ar_write_marker("EA - AGM service startup");
    sp<IAGM> service = new AGM();
    AGM *temp = static_cast<AGM *>(service.get());
    if (temp->is_agm_initialized()) {
        while (!checkBinderServiceReady()) {
            usleep(5000);
        }
        configureRpcThreadpool(16, true /*callerWillJoin*/);
        if(android::OK !=  service->registerAsService()) {
          AGM_LOGE("Could not register AGM service");
          ar_write_marker("EA - AGM service could not register");
          return 1;
        } else {
          AGM_LOGI("AGM service ready");
          ar_write_marker("EA - AGM service ready");
        }
        joinRpcThreadpool();
    }
    return 1;
};
