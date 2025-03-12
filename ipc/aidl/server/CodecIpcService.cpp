/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AgmIpc::codecIpcService"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include "CodecIpcServerWrapper.h"

using namespace aidl::vendor::qti::hardware::agm;

extern "C" __attribute__((visibility("default"))) binder_status_t registerCodecIpcService() {
    ALOGI("register CodecIpc Service");
    auto codecIpcService = ::ndk::SharedRefBase::make<CodecIpcServerWrapper>();
    ndk::SpAIBinder codecBinder = codecIpcService->asBinder();
    const std::string interfaceName = std::string() + ICodecIpc::descriptor + "/default";
    if (!codecIpcService->isInitialized()) {
        ALOGE("failed to initialize Codec Ipc Service!");
        return -EINVAL;
    }

    if (!AServiceManager_isDeclared(interfaceName.c_str())) {
        ALOGW("%s interface %s is not declared in VINTF", __func__, interfaceName.c_str());
    }

    binder_status_t status = AServiceManager_addService(codecBinder.get(), interfaceName.c_str());
    ALOGI("register CodecIpc Service interface %s registered %s status %d", interfaceName.c_str(),
          (status == STATUS_OK) ? "yes" : "no", status);
    return status;
}
