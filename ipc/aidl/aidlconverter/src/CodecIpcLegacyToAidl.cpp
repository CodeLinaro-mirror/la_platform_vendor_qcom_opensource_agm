/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "CodecIpc::LegacyToAidl::Converter"
#include <agm/CodecIpcLegacyToAidl.h>
#include <log/log.h>


CodecModulePayloadList CodecIpcLegacyToAidl::convertCodecModulePayloadListToAidl(struct codec_module_payload_list *mp_list)
{
	struct codec_module_payload *mp = NULL;
	CodecModulePayloadList aidlPaylodList;

	if ((mp_list != NULL) && (mp_list->mp_arr != NULL) && (mp_list->num_mp != 0)) {
		aidlPaylodList.mp.resize(mp_list->num_mp);
		aidlPaylodList.totalPayloadSize = mp_list->total_payload_size;
		for (unsigned int i = 0; i < mp_list->num_mp; i++) {
			mp = mp_list->mp_arr + i;
			aidlPaylodList.mp[i].data.resize(mp->size);
			aidlPaylodList.mp[i].mid = mp->mid;
			memcpy(aidlPaylodList.mp[i].data.data(), mp->data, mp->size);
		}
	}
	return std::move(aidlPaylodList);
}

CodecMediaConfig CodecIpcLegacyToAidl::convertCodecMediaConfigToAidl(struct codec_media_config *legacyConfig)
{
    CodecMediaConfig aidlConfig;
    aidlConfig.rate = legacyConfig->rate;
    aidlConfig.channels = legacyConfig->ch;
    aidlConfig.bitWidth = legacyConfig->bit_width;
    aidlConfig.format = static_cast<CodecMediaFormat>(legacyConfig->format);

    return aidlConfig;
}

