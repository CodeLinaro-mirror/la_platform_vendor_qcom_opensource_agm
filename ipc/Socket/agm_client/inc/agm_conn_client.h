/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _AGM_CONN_CLIENT_H_
#define _AGM_CONN_CLIENT_H_

#include <agm/agm_api.h>

int agm_get_aif_info_list_socket(struct aif_info *aif_list,
                            size_t *num_aif_info);

int agm_get_group_aif_info_list_socket(struct aif_info *aif_list,
                            size_t *num_groups);

int agm_session_set_metadata_socket(uint32_t session_id,
                            uint32_t size,
                            uint8_t *metadata);

int agm_session_aif_get_tag_module_info_socket(uint32_t session_id,
                            uint32_t aif_id,
                            void *payload,
                            size_t *size);

int agm_session_aif_set_params_socket(uint32_t session_id,
                            uint32_t aif_id,
                            void* payload,
                            size_t size);

int agm_aif_set_media_config_socket(uint32_t aif_id,
                            struct agm_media_config *media_config);

int agm_aif_set_metadata_socket(uint32_t aif_id,
                            uint32_t size,
                            uint8_t *metadata);

int agm_session_aif_set_metadata_socket(uint32_t session_id,
                            uint32_t aif_id,
                            uint32_t size,
                            uint8_t *metadata);

int agm_session_aif_connect_socket(uint32_t session_id,
                            uint32_t aif_id,
                            bool state);

int agm_session_open_socket(uint32_t session_id,
                            enum agm_session_mode sess_mode,
                            uint64_t *handle);

int agm_session_set_config_socket(uint64_t hndl,
                            struct agm_session_config *session_config,
                            struct agm_media_config *media_config,
                            struct agm_buffer_config *buffer_config);

int agm_session_prepare_socket(uint64_t hndl);

int agm_session_start_socket(uint64_t hndl);

int agm_session_stop_socket(uint64_t hndl);

int agm_session_close_socket(uint64_t hndl);

int agm_session_write_socket(uint64_t hndl,
                            void *buff,
                            size_t *count);

int agm_hw_rsc_config_socket(enum agm_hw_config_type type,
                            uint8_t *cfg,
                            uint32_t cfg_len,
                            uint8_t *outbuff,
                            uint32_t *out_len);

#endif
