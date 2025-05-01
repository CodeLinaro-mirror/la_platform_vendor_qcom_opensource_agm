/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "CodecIpc::Client"

#include <aidl/vendor/qti/hardware/agm/ICodecIpc.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include <agm/CodecIpcLegacyToAidl.h>
#include <agm/AgmLegacyToAidl.h>
#include <agm/BinderStatus.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <agm/codec/codec_interface.h>

using ::aidl::vendor::qti::hardware::agm::ICodecIpc;

static std::shared_ptr<ICodecIpc> gCodecIpcClient = nullptr;
static ::ndk::ScopedAIBinder_DeathRecipient gDeathRecipient;
static std::mutex gLock;

#define RETURN_IF_CODEC_IPC_SERVICE_NOT_REGISTERED(client)            \
    ({                                                         \
        if (client.get() == nullptr) {                         \
            ALOGE(" %s CodecIpc service doesn't exist ", __func__); \
            return -EINVAL;                                    \
        }                                                      \
    })

void serviceDied(void *cookie) {
    ALOGE("%s : CodecIpc Service died ,cookie : %llu", __func__, (unsigned long long)cookie);
    std::lock_guard<std::mutex> guard(gLock);
    gCodecIpcClient = nullptr;
}

std::shared_ptr<ICodecIpc> getCodecIpc() {
    std::lock_guard<std::mutex> guard(gLock);
    if (gCodecIpcClient == nullptr) {
        const std::string instance = std::string() + ICodecIpc::descriptor + "/default";
        ABinderProcess_startThreadPool();
        auto binder = ::ndk::SpAIBinder(AServiceManager_waitForService(instance.c_str()));
        ALOGV("%s got binder %p", __func__, binder.get());

        auto newClient = ICodecIpc::fromBinder(binder);

        if (newClient == nullptr) {
            ALOGE("could not get CodecIpcClient fromBinder");
            return nullptr;
        }
        gCodecIpcClient = newClient;
        ALOGI("%s gCodecIpcClient %p ", __func__, gCodecIpcClient.get());

        gDeathRecipient =
                ::ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(&serviceDied));
        auto status = ::ndk::ScopedAStatus::fromStatus(
                AIBinder_linkToDeath(binder.get(), gDeathRecipient.get(), (void *)serviceDied));

        if (!status.isOk()) {
            ALOGV("linking service to death failed: %d: %s", status.getStatus(),
                  status.getMessage());
        } else {
            ALOGI("linked to death %d: %s", status.getStatus(), status.getMessage());
        }
    }
    ALOGV("%s gCodecIpcClient %p ", __func__, gCodecIpcClient.get());
    return gCodecIpcClient;
}

int cdc_interface_init()
{
	return 0;
}

int cdc_interface_deinit()
{
	return 0;
}

int cdc_route_endpoint(int ep_id, struct codec_module_payload_list *mp_list, bool set)
{
	auto client = getCodecIpc();
    RETURN_IF_CODEC_IPC_SERVICE_NOT_REGISTERED(client);

	auto aidlConfig = CodecIpcLegacyToAidl::convertCodecModulePayloadListToAidl(mp_list);
	return statusTFromBinderStatus(client->ipc_cdc_route_endpoint(ep_id, aidlConfig, set));
}

int cdc_enable_endpoint(int ep_id, struct codec_media_config *config)
{
	auto client = getCodecIpc();
    RETURN_IF_CODEC_IPC_SERVICE_NOT_REGISTERED(client);

	auto aidlConfig = CodecIpcLegacyToAidl::convertCodecMediaConfigToAidl(config);
	return statusTFromBinderStatus(client->ipc_cdc_enable_endpoint(ep_id, aidlConfig));
}

int cdc_disable_endpoint(int ep_id)
{
	auto client = getCodecIpc();
    RETURN_IF_CODEC_IPC_SERVICE_NOT_REGISTERED(client);

	return statusTFromBinderStatus(client->ipc_cdc_disable_endpoint(ep_id));
}

int cdc_set_custom_payload_endpoint(int cdc_param_id, uint32_t payload_size, uint8_t *ep_payload)
{
	auto client = getCodecIpc();
    RETURN_IF_CODEC_IPC_SERVICE_NOT_REGISTERED(client);

	auto aidlPayload = LegacyToAidl::convertRawPayloadToVector(ep_payload, payload_size);
	return statusTFromBinderStatus(client->ipc_cdc_set_custom_payload_endpoint(cdc_param_id, aidlPayload));
}
