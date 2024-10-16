/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "agm_conn_client"
#include <agm_connection.h>
#include <agm_conn_client.h>
#include <log/log.h>

using namespace std;

int agm_get_aif_info_list_socket(struct aif_info *aif_list,
                            size_t *num_aif_info)
{
    int32_t ret = 0;
    uint32_t info_num = 0;

    ALOGV("%s", __func__);
    /* 1. get socket client */
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = 0;
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        ALOGV("Nothing to fill");
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_GET_AIF_INFO_LIST,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret, &aif_list, &info_num, num_aif_info](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_GET_AIF_INFO_LIST == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
            payload += sizeof(int32_t);
            memcpy(&info_num, payload, sizeof(uint32_t));
            payload += sizeof(uint32_t);
            if (aif_list) {
                auto info = (struct AifInfo_Socket*)payload;
                for (uint32_t i = 0; i < info_num; i++) {
                    strlcpy(aif_list[i].aif_name,
                            info[i].aif_name,
                            AIF_NAME_MAX_LEN);
                    aif_list[i].dir = (enum direction)info[i].dir;
                }
            }
        }
        *num_aif_info = (size_t)info_num;
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_get_group_aif_info_list_socket(struct aif_info *aif_list,
                            size_t *num_groups)
{
    int32_t ret = 0;
    uint32_t info_num = 0;

    ALOGV("%s", __func__);
    /* 1. get socket client */
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = 0;
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        ALOGV("Nothing to fill");
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_GET_GROUP_AIF_INFO_LIST,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret, &aif_list, &info_num, num_groups](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_GET_GROUP_AIF_INFO_LIST == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
            payload += sizeof(int32_t);
            memcpy(&info_num, payload, sizeof(uint32_t));
            payload += sizeof(uint32_t);
            if (aif_list) {
                auto info = (struct AifInfo_Socket*)payload;
                for (uint32_t i = 0; i < info_num; i++) {
                    strlcpy(aif_list[i].aif_name,
                            info[i].aif_name,
                            AIF_NAME_MAX_LEN);
                    aif_list[i].dir = (enum direction)info[i].dir;
                }
            }
            *num_groups = (size_t)info_num;
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_set_metadata_socket(uint32_t session_id,
                            uint32_t size,
                            uint8_t *metadata)
{
    int32_t ret = 0;
    ALOGV("%s", __func__);
    /* 1. get socket client */
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) +
                            sizeof(uint32_t) +
                            size;
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &session_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &size, sizeof(uint32_t));
        payload += sizeof(size);
        memcpy(payload, metadata, size);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_SET_METADATA,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_SET_METADATA == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_aif_get_tag_module_info_socket(uint32_t session_id,
                            uint32_t aif_id,
                            void *ret_payload,
                            size_t *ret_size)
{
    int32_t ret = 0;
    uint32_t temp_size = 0;

    ALOGV("%s", __func__);
    /* 1. get socket client */
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) +
                            sizeof(uint32_t);

    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &session_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &aif_id, sizeof(uint32_t));
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_AIF_GET_TAG_MODULE_INFO,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret, &ret_payload, &temp_size, ret_size](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_AIF_GET_TAG_MODULE_INFO == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
            payload += sizeof(int32_t);
            memcpy(&temp_size, payload, sizeof(uint32_t));
            payload += sizeof(uint32_t);
            if (ret_payload) {
                memcpy(ret_payload, payload, temp_size);
            }
            *ret_size = (size_t)temp_size;
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_aif_set_params_socket(uint32_t session_id,
                            uint32_t aif_id,
                            void* input_payload,
                            size_t size)
{
    int32_t ret = 0;
    uint32_t temp_size = (uint32_t) size;

    ALOGV("%s", __func__);
    /* 1. get socket client */
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) +
                            sizeof(uint32_t) +
                            sizeof(uint32_t) +
                            temp_size;
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &session_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &aif_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &temp_size, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, input_payload, temp_size);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_AIF_SET_PARAMS,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_AIF_SET_PARAMS == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_aif_set_cal_socket(uint32_t session_id,uint32_t aif_id,
                            struct agm_cal_config *cal_config)
{
    int32_t ret = 0;
    struct AgmCalConfig_Socket *cconfig = nullptr;
    ALOGD("%s", __func__);
    /* 1. get socket client */
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) + sizeof(uint32_t) +
                            sizeof(struct AgmCalConfig_Socket);
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &session_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &aif_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);

        struct AgmCalConfig_Socket *cconfig =
                  (struct AgmCalConfig_Socket*)malloc(sizeof(struct AgmCalConfig_Socket) +
                               (cal_config->num_ckvs * sizeof(struct AgmKeyValue_Socket)));

        cconfig->num_ckvs = cal_config->num_ckvs;

        for (int i=0 ; i < cal_config->num_ckvs ; i++ ) {
            cconfig->kv[i].key = cal_config->kv[i].key;
            cconfig->kv[i].value = cal_config->kv[i].value;
            ALOGV("Debug  session_id %u aif_id %u num_ckvs %u,key %u value %u",
                            session_id, aif_id, cal_config->num_ckvs,
                            cal_config->kv[i].key, cal_config->kv[i].value);
        }

        memcpy(payload, cconfig, sizeof(struct AgmCalConfig_Socket));

    };
    ALOGD("Debug  session_id %u aif_id %u num_ckvs %u", session_id, aif_id, cal_config->num_ckvs);

    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_AIF_SET_CAL,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size,
                    uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_AIF_SET_CAL == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    /* 7. Free allocated memory and return */
    free(cconfig);
    return ret;
}

int agm_aif_set_media_config_socket(uint32_t aif_id,
                struct agm_media_config *media_config)
{
    int32_t ret = 0;
    ALOGV("%s", __func__);
    /* 1. get socket client */
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) +
                            sizeof(struct AgmMediaConfig_Socket);
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &aif_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);

        struct AgmMediaConfig_Socket config = {
            .rate = media_config->rate,
            .channels = media_config->channels,
            .format = media_config->format,
            .data_format = media_config->data_format,
        };

        memcpy(payload, &config, sizeof(struct AgmMediaConfig_Socket));
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_AIF_SET_MEDIA_CONFIG,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_AIF_SET_MEDIA_CONFIG == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_aif_set_metadata_socket(uint32_t aif_id,
                            uint32_t size,
                            uint8_t *metadata)
{
    int32_t ret = 0;
    ALOGV("%s", __func__);
    /* 1. get socket client */
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) +
                            sizeof(uint32_t) +
                            size;
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &aif_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &size, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, metadata, size);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_AIF_SET_METADATA,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_AIF_SET_METADATA == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_aif_set_metadata_socket(uint32_t session_id,
                            uint32_t aif_id,
                            uint32_t size,
                            uint8_t *metadata)
{
    int32_t ret = 0;
    ALOGV("%s", __func__);
    /* 1. get socket client */
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) +
                            sizeof(uint32_t) +
                            sizeof(uint32_t) +
                            size;
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &session_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &aif_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &size, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, metadata, size);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_AIF_SET_METADATA,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_AIF_SET_METADATA == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_aif_connect_socket(uint32_t session_id,
                            uint32_t aif_id,
                            bool state)
{
    int32_t ret = 0;
    /* 1. get socket client */
    ALOGV("%s", __func__);
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) +
                            sizeof(uint32_t) +
                            sizeof(bool);
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &session_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &aif_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &state, sizeof(bool));
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_AIF_CONNECT,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_AIF_CONNECT == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_open_socket(uint32_t session_id,
                            enum agm_session_mode sess_mode,
                            uint64_t *handle)
{
    int32_t ret = 0;
    /* 1. get socket client */
    ALOGV("%s", __func__);
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) +
                            sizeof(uint32_t);
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &session_id, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        uint32_t mode = sess_mode;
        memcpy(payload, &mode, sizeof(uint32_t));
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_OPEN,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret, handle](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_OPEN == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
            payload += sizeof(int32_t);
            memcpy(handle, payload, sizeof(uint64_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_set_config_socket(uint64_t hndl,
                            struct agm_session_config *session_config,
                            struct agm_media_config *media_config,
                            struct agm_buffer_config *buffer_config)
{
    int32_t ret = 0;
    /* 1. get socket client */
    ALOGV("%s", __func__);
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint64_t) +
                            sizeof(struct AgmSessionConfig_Socket) +
                            sizeof(struct AgmMediaConfig_Socket) +
                            sizeof(struct AgmBufferConfig_Socket);
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &hndl, sizeof(uint64_t));
        payload += sizeof(uint64_t);

        struct AgmSessionConfig_Socket sConfig = {
            .dir = session_config->dir,
            .sess_mode = session_config->sess_mode,
            .start_threshold = session_config->start_threshold,
            .stop_threshold = session_config->stop_threshold,
            .codec = session_config->codec,
            .data_mode = session_config->data_mode,
            .sess_flags = session_config->sess_flags,
        };
        memcpy(payload, &sConfig, sizeof(struct AgmSessionConfig_Socket));
        payload += sizeof(struct AgmSessionConfig_Socket);

        struct AgmMediaConfig_Socket mConfig = {
            .rate = media_config->rate,
            .channels = media_config->channels,
            .format = media_config->format,
            .data_format = media_config->data_format,
        };
        memcpy(payload, &mConfig, sizeof(struct AgmMediaConfig_Socket));
        payload += sizeof(struct AgmMediaConfig_Socket);

        struct AgmBufferConfig_Socket bConfig = {
            .count = buffer_config->count,
            .size = (uint32_t)buffer_config->size,
            .max_metadata_size = (uint32_t)buffer_config->max_metadata_size,
        };
        memcpy(payload, &bConfig, sizeof(struct AgmBufferConfig_Socket));
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_SET_CONFIG,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_SET_CONFIG == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_prepare_socket(uint64_t hndl)
{
    int32_t ret = 0;
    /* 1. get socket client */
    ALOGV("%s", __func__);
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint64_t);
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &hndl, sizeof(uint64_t));
        payload += sizeof(uint64_t);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_PREPARE,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_PREPARE == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_start_socket(uint64_t hndl)
{
    int32_t ret = 0;
    /* 1. get socket client */
    ALOGV("%s", __func__);
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint64_t);
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &hndl, sizeof(uint64_t));
        payload += sizeof(uint64_t);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_START,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_START == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_stop_socket(uint64_t hndl)
{
    int32_t ret = 0;
    /* 1. get socket client */
    ALOGV("%s", __func__);
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint64_t);
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &hndl, sizeof(uint64_t));
        payload += sizeof(uint64_t);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_STOP,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_STOP == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_close_socket(uint64_t hndl)
{
    int32_t ret = 0;
    /* 1. get socket client */
    ALOGV("%s", __func__);
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint64_t);
    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &hndl, sizeof(uint64_t));
        payload += sizeof(uint64_t);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_CLOSE,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_CLOSE == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_session_write_socket(uint64_t hndl,
                            void *buff,
                            size_t *count)
{
    int32_t ret = 0;
    uint32_t temp_size = (uint32_t)*count;
    /* 1. get socket client */
    ALOGV("%s", __func__);
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint64_t) +
                            sizeof(uint32_t) +
                            temp_size;

    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &hndl, sizeof(uint64_t));
        payload += sizeof(uint64_t);
        memcpy(payload, &temp_size, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, buff, temp_size);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_SESSION_WRITE,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret, &temp_size, count](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_SESSION_WRITE == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
            payload += sizeof(int32_t);
            memcpy(&temp_size, payload, sizeof(uint32_t));
        }
        *count = (size_t)temp_size;
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}

int agm_hw_rsc_config_socket(enum agm_hw_config_type type,
                            uint8_t *cfg,
                            uint32_t cfg_len,
                            uint8_t *outbuff,
                            uint32_t *out_len)
{
    int32_t ret = 0;
    uint32_t config_type = (uint32_t)type;
    /* 1. get socket client */
    ALOGV("%s", __func__);
    AgmSocketClient* conn = AgmSocketClient::getInstance();
    /* 2. get whole payload size */
    uint32_t payload_size = sizeof(uint32_t) +
                            sizeof(uint32_t) +
                            sizeof(uint32_t) +
                            cfg_len;

    /* 3. define function obj to write payload */
    auto payloadFiller = [&](uint8_t* payload) {
        memcpy(payload, &config_type, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, &cfg_len, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, out_len, sizeof(uint32_t));
        payload += sizeof(uint32_t);
        memcpy(payload, cfg, cfg_len);
    };
    /* 4. call Send for IPC */
    conn->Send(AGM_CMD_HW_SRC_CONFIG,
            AGM_CMD_TYPE_REQUEST,
            payload_size,
            payloadFiller);
    /* 5. define function obj to extract reply from payload */
    auto replyhandler = [&ret, out_len, outbuff](uint16_t cmd,uint16_t msg_type,uint32_t size, uint8_t* payload, const AgmSocket& socket) {
        if (AGM_CMD_HW_SRC_CONFIG == cmd && AGM_CMD_TYPE_REPLY == msg_type) {
            memcpy(&ret, payload, sizeof(int32_t));
            payload += sizeof(int32_t);
            memcpy(out_len, payload, sizeof(uint32_t));
            payload += sizeof(uint32_t);
            if (!ret && *out_len > 0)
                memcpy(outbuff, payload, *out_len);
        }
    };
    /* 6. call Receive to get IPC reply */
    conn->Receive(replyhandler);

    return ret;
}
