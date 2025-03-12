/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#define LOG_TAG "CodecIpc::Server"

#include <agm/AgmAidlToLegacy.h>
#include <agm/CodecIpcLegacyToAidl.h>
#include <agm/Utils.h>

#include "CodecIpcServerWrapper.h"

#include <agm/BinderStatus.h>
#include <agm/codec/codec_interface.h>

using ndk::ScopedAStatus;

namespace aidl::vendor::qti::hardware::agm {

CodecIpcServerWrapper::CodecIpcServerWrapper() {
    mInitialized = (cdc_interface_init() == 0);
    ALOGI("%s created", __func__);
}

CodecIpcServerWrapper::~CodecIpcServerWrapper() {
    ALOGI("%s destroyed", __func__);
}

::ndk::ScopedAStatus CodecIpcServerWrapper::ipc_cdc_interface_init() {
    ALOGV("%s ", __func__);
    return ScopedAStatus::ok();
}
::ndk::ScopedAStatus CodecIpcServerWrapper::ipc_cdc_iterface_deinit() {
    ALOGV("%s ", __func__);
    return ScopedAStatus::ok();
}
::ndk::ScopedAStatus CodecIpcServerWrapper::ipc_cdc_route_endpoint(int32_t in_ep_id,
						const ::aidl::vendor::qti::hardware::agm::CodecModulePayloadList &in_mp_list,
						bool in_set) {

	int32_t ret = 0;
	struct codec_module_payload_list mp_list;
	struct codec_module_payload *mp = NULL;

	memset(&mp_list, 0, sizeof(mp_list));
	mp_list.total_payload_size = in_mp_list.totalPayloadSize;
	mp_list.num_mp = in_mp_list.mp.size();
	mp_list.mp_arr = (struct codec_module_payload*)calloc(mp_list.num_mp *
                        sizeof(struct codec_module_payload), 1);

	if (!mp_list.mp_arr) {
		ret = -ENOMEM;
		goto exit;
	}

	for (int32_t idx = 0; idx < mp_list.num_mp; idx++)
	{
		mp = mp_list.mp_arr + idx;
		mp->mid = in_mp_list.mp[idx].mid;
		mp->size = in_mp_list.mp[idx].data.size();
		mp->data = (uint8_t*)calloc(mp->size, 1);\
		if (!mp->data) {
			ret = -ENOMEM;
			goto exit;
		}
		memcpy(mp->data, in_mp_list.mp[idx].data.data(), mp->size);
	}

	ret = cdc_route_endpoint(in_ep_id, &mp_list, in_set);

exit:
	for (int32_t idx = 0; idx < mp_list.num_mp; idx++) {
		if (mp_list.mp_arr) {
			mp = mp_list.mp_arr + idx;
			if (mp->data)
				free(mp->data);
		}
	}
	if (mp_list.mp_arr)
		free(mp_list.mp_arr);

	return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus CodecIpcServerWrapper::ipc_cdc_enable_endpoint(int32_t in_ep_id,
						const ::aidl::vendor::qti::hardware::agm::CodecMediaConfig &in_media_config) {

	struct codec_media_config legacy_config;

	legacy_config.rate = in_media_config.rate;
    legacy_config.ch = in_media_config.channels;
	legacy_config.bit_width = in_media_config.bitWidth;
    legacy_config.format = static_cast<codec_media_format_t>(in_media_config.format);

	return status_tToBinderResult(cdc_enable_endpoint(in_ep_id, &legacy_config));
}

::ndk::ScopedAStatus CodecIpcServerWrapper::ipc_cdc_disable_endpoint(int32_t in_ep_id) {

    return status_tToBinderResult(cdc_disable_endpoint(in_ep_id));
}

::ndk::ScopedAStatus CodecIpcServerWrapper::ipc_cdc_set_custom_payload_endpoint(int32_t in_cdc_param_id,
						const std::vector<uint8_t> &in_ep_payload)  {
	int32_t ret = 0;

	auto LegacyPayload =
            VALUE_OR_RETURN(allocate<uint8_t>(in_ep_payload.size()));

	memcpy(LegacyPayload.get(), in_ep_payload.data(), in_ep_payload.size());

	ret = cdc_set_custom_payload_endpoint(in_cdc_param_id, LegacyPayload.get(), in_ep_payload.size());
    return status_tToBinderResult(ret);
}

}
