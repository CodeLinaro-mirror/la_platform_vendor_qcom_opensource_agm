/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/vendor/qti/hardware/agm/CodecModulePayload.h>
#include <aidl/vendor/qti/hardware/agm/CodecModulePayloadList.h>
#include <aidl/vendor/qti/hardware/agm/CodecMediaConfig.h>
#include <aidl/vendor/qti/hardware/agm/CodecMediaFormat.h>

#include <agm/codec/codec_interface.h>

using namespace ::aidl::vendor::qti::hardware::agm;

struct CodecIpcLegacyToAidl {

    static CodecModulePayloadList convertCodecModulePayloadListToAidl(struct codec_module_payload_list *mp_list);

    static CodecMediaConfig convertCodecMediaConfigToAidl(struct codec_media_config *config);

};
