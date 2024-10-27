/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "agm_conn_server"
#include <agm_connection.h>
#include "inc/agm_conn_server.h"
#include <log/log.h>
#include <agm/agm_api.h>
#ifdef AGM_HW_RSC_CFG_EN
#include "gsl_hw_rsc_intf.h"
#endif
#include <unordered_map>

using namespace std;

AgmSocketServer* gAgmServer = nullptr;

static int32_t executeCmd(const AgmSocket& conn);
static void agm_aif_set_media_config_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_aif_set_metadata_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_aif_set_metadata_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_aif_connect_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_open_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_set_config_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_prepare_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_start_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_stop_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_close_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_get_aif_info_list_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_get_group_aif_info_list_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_set_metadata_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_aif_get_tag_module_info_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_aif_set_params_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_session_write_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);
static void agm_hw_rsc_config_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn);

int init_service_socket() {
    ALOGV("%s", __func__);
    gAgmServer = AgmSocketServer::getInstance(&executeCmd);
    return 0;
}

int deinit_service_socket() {
    ALOGV("%s", __func__);
    AgmSocketServer::releaseInstance();
    return 0;
}

using agmServerWrapper = function<void(uint16_t, uint32_t, uint8_t*, const AgmSocket&)>;
static unordered_map<uint16_t, agmServerWrapper> functionTable = {
    {static_cast<uint16_t>(AGM_CMD_AIF_SET_MEDIA_CONFIG), &agm_aif_set_media_config_socket},
    {static_cast<uint16_t>(AGM_CMD_AIF_SET_METADATA), &agm_aif_set_metadata_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_AIF_SET_METADATA), &agm_session_aif_set_metadata_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_AIF_CONNECT), &agm_session_aif_connect_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_OPEN), &agm_session_open_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_SET_CONFIG), &agm_session_set_config_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_PREPARE), &agm_session_prepare_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_START), &agm_session_start_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_STOP), &agm_session_stop_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_CLOSE), &agm_session_close_socket},
    {static_cast<uint16_t>(AGM_CMD_GET_AIF_INFO_LIST), &agm_get_aif_info_list_socket},
    {static_cast<uint16_t>(AGM_CMD_GET_GROUP_AIF_INFO_LIST), &agm_get_group_aif_info_list_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_SET_METADATA), &agm_session_set_metadata_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_AIF_GET_TAG_MODULE_INFO), &agm_session_aif_get_tag_module_info_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_AIF_SET_PARAMS), &agm_session_aif_set_params_socket},
    {static_cast<uint16_t>(AGM_CMD_SESSION_WRITE), &agm_session_write_socket},
    {static_cast<uint16_t>(AGM_CMD_HW_SRC_CONFIG), &agm_hw_rsc_config_socket}
};

static void agm_conn_command_handler(uint16_t cmd,
                uint16_t msg_type __unused,
                uint32_t payload_size,
                uint8_t* payload,
                const AgmSocket& conn) {

    ALOGV("%s", __func__);
    auto func_wrapper_itr = functionTable.find(cmd);

    if (func_wrapper_itr == functionTable.end()) {
        ALOGE("No function availale for cmd %d", cmd);
        return;
    }

    func_wrapper_itr->second(cmd, payload_size, payload, conn);
}

static int32_t executeCmd(const AgmSocket& conn) {
    ALOGV("%s", __func__);
    return conn.Receive(&agm_conn_command_handler);
}

static void agm_aif_set_media_config_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint32_t* aif_id = nullptr;
    struct AgmMediaConfig_Socket* media_config = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(struct AgmMediaConfig_Socket))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    aif_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    media_config = reinterpret_cast<struct AgmMediaConfig_Socket*>(pos);
    ALOGV("Debug aif_id %u rate %u ch %u format %d data_format %u",
            *aif_id, media_config->rate, media_config->channels, media_config->format,
            media_config->data_format);

    /* 3. call agm service interface */
    struct agm_media_config mConfig = {
        .rate = media_config->rate,
        .channels = media_config->channels,
        .format = (enum agm_media_format)media_config->format,
        .data_format = media_config->data_format,
    };
    ret = agm_aif_set_media_config(*aif_id, &mConfig);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_aif_set_metadata_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint32_t* aif_id = nullptr;
    uint32_t* size = nullptr;
    uint8_t* metadata = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    aif_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    size = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    metadata = pos;
    /* function special handling for variable arg length */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t) + *size)) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    ALOGV("Debug aif_id %u size %u ", *aif_id, *size);

    /* 3. call agm service interface */
    ret = agm_aif_set_metadata(*aif_id, *size, metadata);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_aif_set_metadata_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint32_t* session_id = nullptr;
    uint32_t* aif_id = nullptr;
    uint32_t* size = nullptr;
    uint8_t* metadata = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    session_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    aif_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    size = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    metadata = pos;
    /* function special handling for variable arg length */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + *size)) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    ALOGV("Debug session_id %u aif_id %u size %u ", *session_id, *aif_id, *size);

    /* 3. call agm service interface */
    ret = agm_session_aif_set_metadata(*session_id, *aif_id, *size, metadata);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_aif_connect_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint32_t* session_id = nullptr;
    uint32_t* aif_id = nullptr;
    bool* state = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t)+ sizeof(bool))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    session_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    aif_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    state = reinterpret_cast<bool*>(pos);
    ALOGV("Debug session_id %u aif_id %u state %d", *session_id, *aif_id, *state);


    /* 3. call agm service interface */
    ret = agm_session_aif_connect(*session_id, *aif_id, *state);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_open_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint32_t* session_id = nullptr;
    uint32_t* sess_mode = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint64_t handle = 0; /* return to client */
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret, &handle](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
            payload += sizeof(ret);
            memcpy(payload, &handle, sizeof(handle));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(ret) + sizeof(handle), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    session_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    sess_mode = reinterpret_cast<uint32_t*>(pos);
    ALOGV("Debug session_id %u sess_mode %d", *session_id, *sess_mode);


    /* 3. call agm service interface */
    auto mode = (enum agm_session_mode)*sess_mode;
    ret = agm_session_open(*session_id, mode, &handle);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret) + sizeof(handle);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret, &handle](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
        payload += sizeof(ret);
        memcpy(payload, &handle, sizeof(uint64_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_set_config_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint64_t* handle = nullptr;
    struct AgmSessionConfig_Socket *session_config = nullptr;
    struct AgmMediaConfig_Socket *media_config = nullptr;
    struct AgmBufferConfig_Socket *buffer_config = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint64_t) +
                sizeof(struct AgmSessionConfig_Socket) +
                sizeof(struct AgmMediaConfig_Socket) +
                sizeof(struct AgmBufferConfig_Socket))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    handle = reinterpret_cast<uint64_t*>(pos);
    pos += sizeof(uint64_t);
    session_config = reinterpret_cast<struct AgmSessionConfig_Socket*>(pos);
    pos += sizeof(struct AgmSessionConfig_Socket);
    media_config = reinterpret_cast<struct AgmMediaConfig_Socket*>(pos);
    pos += sizeof(struct AgmMediaConfig_Socket);
    buffer_config = reinterpret_cast<struct AgmBufferConfig_Socket*>(pos);
    ALOGV("Debug handle %llu", *handle);

    struct agm_session_config sConfig = {
        .dir = (enum direction)session_config->dir,
        .sess_mode = (enum agm_session_mode)session_config->sess_mode,
        .start_threshold = session_config->start_threshold,
        .stop_threshold = session_config->stop_threshold,
        .codec = session_config->codec,
        .data_mode = (enum agm_data_mode)session_config->data_mode,
        .sess_flags = session_config->sess_flags,
    };

    struct agm_media_config mConfig = {
        .rate = media_config->rate,
        .channels = media_config->channels,
        .format = (enum agm_media_format)media_config->format,
        .data_format = media_config->data_format,
    };

    struct agm_buffer_config bConfig = {
        .count = buffer_config->count,
        .size = buffer_config->size,
        .max_metadata_size = buffer_config->max_metadata_size,
    };

    /* 3. call agm service interface */
    ret = agm_session_set_config(*handle, &sConfig, &mConfig, &bConfig);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}
static void agm_session_prepare_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint64_t* handle = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint64_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    handle = reinterpret_cast<uint64_t*>(pos);
    ALOGV("Debug handle %llu", *handle);

    /* 3. call agm service interface */
    ret = agm_session_prepare(*handle);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_start_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint64_t* handle = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint64_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    handle = reinterpret_cast<uint64_t*>(pos);
    ALOGV("Debug handle %llu", *handle);

    /* 3. call agm service interface */
    ret = agm_session_start(*handle);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_stop_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint64_t* handle = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint64_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    handle = reinterpret_cast<uint64_t*>(pos);
    ALOGV("Debug handle %llu", *handle);

    /* 3. call agm service interface */
    ret = agm_session_stop(*handle);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_close_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint64_t* handle = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint64_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    handle = reinterpret_cast<uint64_t*>(pos);
    ALOGV("Debug handle %llu", *handle);

    /* 3. call agm service interface */
    ret = agm_session_close(*handle);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_get_aif_info_list_socket(uint16_t cmd,
                uint32_t args_payload_size __unused,
                uint8_t* args_payload __unused,
                const AgmSocket& conn)
{
    int32_t ret = 0;
    uint32_t temp_num_aif_info = 0;
    size_t num_aif_info = 0;
    struct AifInfo_Socket *aif_list = nullptr;
    uint32_t reply_size = 0;

    /* skip 1&2 since no payload for the interface*/

    /* 3. call agm service interface */
    ret = agm_get_aif_info_list(nullptr, &num_aif_info);
    if (!ret) {
        temp_num_aif_info = (uint32_t)num_aif_info;
        auto info = reinterpret_cast<struct aif_info*>(calloc(num_aif_info, sizeof(struct aif_info)));
        aif_list = reinterpret_cast<struct AifInfo_Socket*>(calloc(num_aif_info, sizeof(struct AifInfo_Socket)));
        if (aif_list && info) {
            ret = agm_get_aif_info_list(info, &num_aif_info);
            for (int i=0; i < num_aif_info; i++) {
                strlcpy(aif_list[i].aif_name,
                        info[i].aif_name,
                        AIF_NAME_MAX_LEN);
                aif_list[i].dir = info[i].dir;
            }
        }
        if (info) {
            free(info);
        }
    }

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);
    reply_size += sizeof(temp_num_aif_info);
    reply_size += sizeof(struct AifInfo_Socket) * temp_num_aif_info;
    ALOGV("Debug ret %d aif_num %u return size %zu", ret, temp_num_aif_info, reply_size);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret, &temp_num_aif_info, &aif_list](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(ret));
        payload += sizeof(ret);
        memcpy(payload, &temp_num_aif_info, sizeof(temp_num_aif_info));
        payload += sizeof(temp_num_aif_info);
        if (aif_list) {
            memcpy(payload, aif_list, (sizeof(struct AifInfo_Socket) * temp_num_aif_info));
            free(aif_list);
        }
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_get_group_aif_info_list_socket(uint16_t cmd,
                uint32_t args_payload_size __unused,
                uint8_t* args_payload __unused,
                const AgmSocket& conn)
{
    int32_t ret = 0;
    uint32_t temp_num_groups = 0;
    size_t num_groups = 0;
    struct AifInfo_Socket *aif_list = nullptr;
    uint32_t reply_size = 0;

    /* skip 1&2 since no payload for the interface*/

    /* 3. call agm service interface */
    ret = agm_get_group_aif_info_list(nullptr, &num_groups);
    if (!ret) {
        temp_num_groups = (uint32_t)num_groups;
        auto info = reinterpret_cast<struct aif_info*>(calloc(num_groups, sizeof(struct aif_info)));
        aif_list = reinterpret_cast<struct AifInfo_Socket*>(calloc(num_groups, sizeof(struct AifInfo_Socket)));
        if (aif_list && info) {
            ret = agm_get_group_aif_info_list(info, &num_groups);
            for (int i=0; i < num_groups; i++) {
                strlcpy(aif_list[i].aif_name,
                        info[i].aif_name,
                        AIF_NAME_MAX_LEN);
                aif_list[i].dir = info[i].dir;
            }
        }
        if (info) {
            free(info);
        }
    }

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);
    reply_size += sizeof(temp_num_groups);
    reply_size += sizeof(struct AifInfo_Socket) * temp_num_groups;
    ALOGV("Debug ret %d aif_num %u return size %zu", ret, temp_num_groups, reply_size);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret, &temp_num_groups, &aif_list](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(ret));
        payload += sizeof(ret);
        memcpy(payload, &temp_num_groups, sizeof(temp_num_groups));
        payload += sizeof(temp_num_groups);
        if (aif_list) {
            memcpy(payload, aif_list, (sizeof(struct AifInfo_Socket) * temp_num_groups));
            free(aif_list);
        }
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_set_metadata_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint32_t* session_id = nullptr;
    uint32_t* size = nullptr;
    uint8_t* metadata = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    session_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    size = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    metadata = reinterpret_cast<uint8_t*>(pos);
    ALOGV("Debug session_id %u size %u", *session_id, *size);
    /* specific handling for the interface - validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t) + *size)) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }

    /* 3. call agm service interface */
    ret = agm_session_set_metadata(*session_id, *size, metadata);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_aif_get_tag_module_info_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint32_t* session_id = nullptr;
    uint32_t* aif_id = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    void* ret_payload = nullptr;
    uint32_t temp_size = 0; /* cast to reply format */
    size_t size = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    session_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    aif_id = reinterpret_cast<uint32_t*>(pos);
    ALOGV("Debug session_id %u", *session_id);

    /* 3. call agm service interface */
    ret = agm_session_aif_get_tag_module_info(*session_id, *aif_id, nullptr, &size);
    if (!ret) {
        temp_size = (uint32_t)size;
        ret_payload = calloc(1, size);
        if (ret_payload) {
            ret = agm_session_aif_get_tag_module_info(*session_id, *aif_id, ret_payload, &size);
        }
    }

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);
    reply_size += sizeof(temp_size);
    reply_size += temp_size;

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret, &temp_size, &ret_payload](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(ret));
        payload += sizeof(ret);
        memcpy(payload, &temp_size, sizeof(temp_size));
        payload += sizeof(temp_size);
        if (ret_payload) {
            memcpy(payload, ret_payload, temp_size);
            free(ret_payload);
        }
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_aif_set_params_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint32_t* session_id = nullptr;
    uint32_t* aif_id = nullptr;
    uint32_t* size = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    void* input_payload = nullptr;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    session_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    aif_id = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    size = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    input_payload = pos;
    ALOGV("Debug session_id %u aif_id %u size %zu", *session_id, *aif_id, *size);

    /* specific handling for the interface - validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + *size)) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }

    /* 3. call agm service interface */
    ret = agm_session_aif_set_params(*session_id, *aif_id, input_payload, (size_t)*size);

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

static void agm_session_write_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint64_t* handle = nullptr;
    size_t temp_count = 0; /* cast to agm function needed arg */
    uint32_t* count = nullptr;
    void* buff = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint64_t) + sizeof(uint32_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    handle = reinterpret_cast<uint64_t*>(pos);
    pos += sizeof(uint64_t);
    count = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    buff = pos;
    ALOGV("Debug handle %llu count %zu", *handle, *count);
    temp_count = (size_t)*count;

    /* specific handling for the interface - validate the payload size */
    if (args_payload_size <
                (sizeof(uint64_t) + sizeof(uint32_t) + *count)) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }

    /* 3. call agm service interface */
    ret = agm_session_write(*handle, buff, &temp_count);
    *count = (uint32_t)temp_count;

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);
    reply_size += sizeof(*count);

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret, &count](uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
        payload += sizeof(int32_t);
        memcpy(payload, count, sizeof(uint32_t));
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}

#ifdef AGM_HW_RSC_CFG_EN
static void agm_hw_rsc_config_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
    uint32_t* config_type = nullptr;
    uint32_t* cfg_len = nullptr;
    uint8_t* cfg = nullptr;
    uint32_t* buff_size = nullptr;
    uint8_t* pos = args_payload;
    int32_t ret = 0;
    uint8_t* buff = nullptr;
    uint32_t reply_size = 0;

    /* 1. validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t))) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    /* 2. extract args from request's payload */
    config_type = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    cfg_len = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    buff_size = reinterpret_cast<uint32_t*>(pos);
    pos += sizeof(uint32_t);
    cfg = reinterpret_cast<uint8_t*>(pos);

    /* specific handling for the interface - validate the payload size */
    if (args_payload_size <
                (sizeof(uint32_t) + sizeof(uint32_t) +  sizeof(uint32_t) + *cfg_len)) {
        ALOGE("Invalid payload size %llu", args_payload_size);
        auto errorWriter = [&ret](uint8_t* payload) {
            ret = -1;
            memcpy(payload, &ret, sizeof(int32_t));
        };
        conn.Send(cmd, AGM_CMD_TYPE_REPLY, sizeof(int32_t), errorWriter);
        return;
    }
    buff = (uint8_t*)calloc(1, (size_t)*buff_size);

    /* 3. call agm service interface */
    if (*config_type == AGM_HW_CONFIQ_REQ) {
        ret = gsl_request_hw_rsc_custom_config(cfg, *cfg_len, buff, buff_size);
    } else if ( *config_type == AGM_HW_CONFIQ_REL) {
        ret = gsl_release_hw_rsc_custom_config(cfg, *cfg_len, buff, buff_size);
    }

    /* 4. calculate reply's payload size */
    reply_size += sizeof(ret);
    reply_size += sizeof(*buff_size);
    reply_size += *buff_size;

    /* 5. define function obj to write reply's payload */
    auto payloadWriter = [&ret, buff_size, buff] (uint8_t* payload) {
        memcpy(payload, &ret, sizeof(int32_t));
        payload += sizeof(int32_t);
        memcpy(payload, buff_size, sizeof(uint32_t));
        payload += sizeof(int32_t);
        if (buff && *buff_size > 0) {
            memcpy(payload, buff, *buff_size);
            free(buff);
        }
    };

    /* 6. call Send to get IPC reply */
    conn.Send(cmd, AGM_CMD_TYPE_REPLY, reply_size, payloadWriter);
}
#else
static void agm_hw_rsc_config_socket(uint16_t cmd,
                uint32_t args_payload_size,
                uint8_t* args_payload,
                const AgmSocket& conn)
{
}
#endif
