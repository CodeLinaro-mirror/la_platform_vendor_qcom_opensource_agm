/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/vendor/qti/hardware/agm/BnCodecIpc.h>

#include <log/log.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace aidl::vendor::qti::hardware::agm {

class CodecIpcServerWrapper : public BnCodecIpc {
  public:
    explicit CodecIpcServerWrapper();
    virtual ~CodecIpcServerWrapper();
    bool isInitialized() { return mInitialized; }

    ::ndk::ScopedAStatus ipc_cdc_interface_init() override;
    ::ndk::ScopedAStatus ipc_cdc_iterface_deinit() override;
    ::ndk::ScopedAStatus ipc_cdc_route_endpoint(int32_t in_ep_id, const ::aidl::vendor::qti::hardware::agm::CodecModulePayloadList &in_mp_list, bool in_set) override;
    ::ndk::ScopedAStatus ipc_cdc_enable_endpoint(int32_t in_ep_id, const ::aidl::vendor::qti::hardware::agm::CodecMediaConfig &in_media_config) override;
    ::ndk::ScopedAStatus ipc_cdc_disable_endpoint(int32_t in_ep_id) override;
    ::ndk::ScopedAStatus ipc_cdc_set_custom_payload_endpoint(int32_t in_cdc_param_id, const std::vector<uint8_t> &in_ep_payload) override;

    bool mInitialized = false;
};

}
