/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.  */
/* SPDX-License-Identifier: BSD-3-Clause-Clear */

#define LOG_TAG "agm_ls_fe_wrapper"

#include <utils/Log.h>

#include "ipc_common.h"
#include <agm/utils.h>
#include <agm/agm_api.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_SERVER_WRAPPER
#include <log_utils.h>
#endif

// Persistent client socket
int lSocket = -1;
int lCbSocket = -1;

int write(void *buff, size_t size) {
    int len = 0;
    len = write(lSocket, buff, size);
    if (len < 0) {
        AGM_LOGE("IPC socket write Error: %s (errno=%d)\n", strerror(errno),
                 errno);
    }
    return len;
}

int read(void *buff, size_t size) {
    int len = 0;
    len = recv(lSocket, buff, size, MSG_WAITALL);
    if (len < 0) {
        AGM_LOGE("IPC socket recv Error: %s (errno=%d)\n", strerror(errno),
                 errno);
    }
    return len;
}

int readCb(void *buff, size_t size) {
    int len = 0;
    len = recv(lCbSocket, buff, size, MSG_WAITALL);
    if (len < 0) {
        AGM_LOGE("IPC callback socket recv Error: %s (errno=%d)\n",
                 strerror(errno), errno);
    }
    return len;
}

void connect_to_server() {
    struct sockaddr_un serverAddr = {};
    if (lSocket != -1) {
        AGM_LOGV("socket has already been created.");
        goto end;
    }

    lSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (lSocket == -1) {
        AGM_LOGE("Failed to create Agm client socket");
        goto end;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    strlcpy(serverAddr.sun_path, AGM_SOCKET_PATH,
            sizeof(serverAddr.sun_path) - 1);

    if (connect(lSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) ==
        -1) {
        close(lSocket);
        lSocket = -1;
        AGM_LOGE("Failed to connect to Agm Server");
        goto end;
    }
    AGM_LOGI("Success to connect to Agm Server");
end:
    return;
}

void disconnect_from_server() {
    IPCHeader header = {sizeof(IPCHeader), Opcode::DISCONNECT};
    if (lSocket == -1) {
        AGM_LOGE("socket has not been created");
        goto end;
    }

    write(&header, sizeof(header));
    close(lSocket);
    lSocket = -1;

end:
    return;
}

#define CHECK_PAYLOAD_ADDR(payload, pEnd)                                      \
    ({                                                                         \
        int rst = 0;                                                           \
        if ((payload) != (pEnd)) {                                             \
            rst = -1;                                                          \
            AGM_LOGE("payload is not to the pEnd");                            \
        }                                                                      \
        rst;                                                                   \
    })

void ipc_agm_event_cb(const char *payload, size_t payloadSize) {
    agm_event_cb *cb = NULL;
    uint32_t *session_id = 0;
    struct agm_event_cb_params *event_params = NULL;
    void **client_data = NULL;
    const char *pEnd = payload + payloadSize;

    cb = (agm_event_cb *)payload;
    payload += sizeof(agm_event_cb);
    session_id = (uint32_t *)payload, payload += sizeof(uint32_t);
    event_params = (struct agm_event_cb_params *)payload;
    payload += sizeof(agm_event_cb_params) + event_params->event_payload_size;
    client_data = (void **)payload;
    payload += sizeof(void *);

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    (*cb)(*session_id, event_params, *client_data);
}

void ipc_agm_service_crash_cb(const char *payload, size_t payloadSize) {
    agm_service_crash_cb *cb = NULL;
    uint64_t *cookie = 0;
    const char *pEnd = payload + payloadSize;

    cb = (agm_service_crash_cb *)payload;
    payload += sizeof(agm_service_crash_cb);
    cookie = (uint64_t *)payload;
    payload += sizeof(uint64_t);

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    (*cb)(*cookie);
}

void callback_handler() {
    IPCCbHeader header = {};
    ssize_t bytesRead = 0;
    char *payload = nullptr;
    int payloadSize = 0;
    while (true) {
        bytesRead = readCb(&header, sizeof(header));
        if (bytesRead <= 0) {
            break; // Server disconnected
        }

        payloadSize = header.totalLength - sizeof(header);
        if (payloadSize > 0) {
            payload = (char *)malloc(payloadSize);
            if (!payload) {
                AGM_LOGE("malloc failed");
                goto end;
            }
            bytesRead = readCb(payload, payloadSize);
            if (bytesRead <= 0) {
                goto end;
            }
        }
        AGM_LOGI("recieve cbcode %d payloadSize %d", header.cbcode,
                 payloadSize);

        switch (header.cbcode) {
        case Cbcode::AGM_EVETN_CB:
            ipc_agm_event_cb(payload, payloadSize);
            break;
        case Cbcode::AGM_SERVICE_CRASH_CB:
            ipc_agm_service_crash_cb(payload, payloadSize);
            break;
        default:
            AGM_LOGI("Unknown opcode: %d", static_cast<int>(header.cbcode));
            break;
        }
        free(payload);
        payload = NULL;
    }
end:
    if (payload)
        free(payload);
    AGM_LOGI("Callback Client disconnected");
    close(lCbSocket);
}

void connect_to_cb_server() {
    struct sockaddr_un serverAddr = {};
    if (lCbSocket != -1) {
        AGM_LOGV("Callback socket has already been created.");
        goto end;
    }

    lCbSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (lCbSocket == -1) {
        AGM_LOGE("Failed to create Agm client callback socket");
        goto end;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    strlcpy(serverAddr.sun_path, AGM_CB_SOCKET_PATH,
            sizeof(serverAddr.sun_path) - 1);

    if (connect(lCbSocket, (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) == -1) {
        close(lCbSocket);
        lCbSocket = -1;
        AGM_LOGE("Failed to connect to Agm callback Server");
        goto end;
    }
    AGM_LOGI("Success to connect to Agm callback Server");
    std::thread(callback_handler).detach();
end:
    return;
}

void disconnect_from_cb_server() {
    IPCHeader header = {sizeof(IPCHeader), Opcode::DISCONNECT};
    if (lCbSocket == -1) {
        AGM_LOGE("socket has not been created");
        goto end;
    }

    write(&header, sizeof(header));
    close(lCbSocket);
    lSocket = -1;

end:
    return;
}

void get_agm_server_socket() {
    if (lSocket == -1) {
        connect_to_server();
    }
    if (lCbSocket == -1) {
        connect_to_cb_server();
    }
}

int agm_init() {
    AGM_LOGI("invoke %s", __func__);
    get_agm_server_socket();
    return 0;
}

// Implementation of agm_deinit
int agm_deinit() {
    AGM_LOGI("invoke %s", __func__);
    disconnect_from_server();
    disconnect_from_cb_server();
    return 0;
}

int agm_aif_set_media_config(uint32_t aif_id,
                             struct agm_media_config *media_config) {
    int rst = 0;
    size_t payloadSize = sizeof(aif_id) + sizeof(struct agm_media_config);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_AIF_SET_MEDIA_CONFIG};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&aif_id, sizeof(aif_id));
    write(media_config, sizeof(struct agm_media_config));

    read(&rst, sizeof(rst));
    return rst;
}

int agm_aif_set_metadata(uint32_t audio_intf, uint32_t size,
                         uint8_t *metadata) {
    int rst = 0;
    size_t payloadSize = sizeof(audio_intf) + sizeof(size) + size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_AIF_SET_METADATA};

    AGM_LOGI("invoke %s", __func__);
    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&audio_intf, sizeof(audio_intf));
    write(&size, sizeof(size));
    write(metadata, size);

    read(&rst, sizeof(rst));
    return rst;
}

int agm_session_set_metadata(uint32_t session_id, uint32_t size,
                             uint8_t *metadata) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(size) + size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_SET_METADATA};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&size, sizeof(size));
    write(metadata, size);

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_aif_set_metadata(uint32_t session_id, uint32_t audio_intf,
                                 uint32_t size, uint8_t *metadata) {
    int rst = 0;
    size_t payloadSize =
        sizeof(session_id) + sizeof(audio_intf) + sizeof(size) + size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_AIF_SET_METADATA};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&audio_intf, sizeof(audio_intf));
    write(&size, sizeof(size));
    write(metadata, size);

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_aif_connect(uint32_t session_id, uint32_t audio_intf,
                            bool state) {
    int rst = 0;
    size_t payloadSize =
        sizeof(session_id) + sizeof(audio_intf) + sizeof(state);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_AIF_CONNECT};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&audio_intf, sizeof(audio_intf));
    write(&state, sizeof(state));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_aif_get_tag_module_info(uint32_t session_id, uint32_t aif_id,
                                        void *payload, size_t *size) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(aif_id) + sizeof(size);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_AIF_GET_TAG_MODULE_INFO};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&aif_id, sizeof(aif_id));
    write(size, sizeof(size_t));

    read(&rst, sizeof(rst));
    read(size, sizeof(size_t));
    if (size)
        read(payload, *size);

    return rst;
}

int agm_aif_set_params(uint32_t aif_id, void *payload, size_t size) {
    int rst = 0;
    size_t payloadSize = sizeof(aif_id) + sizeof(size) + size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_AIF_SET_PARAMS};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&aif_id, sizeof(aif_id));
    write(&size, sizeof(size));
    write(payload, size);

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_aif_set_params(uint32_t session_id, uint32_t aif_id,
                               void *payload, size_t size) {
    int rst = 0;
    size_t payloadSize =
        sizeof(session_id) + sizeof(aif_id) + sizeof(size) + size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_AIF_SET_PARAMS};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&aif_id, sizeof(aif_id));
    write(&size, sizeof(size));
    write(payload, size);

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_aif_set_cal(uint32_t session_id, uint32_t audio_intf,
                            struct agm_cal_config *cal_config) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(audio_intf) +
                         sizeof(agm_cal_config) +
                         cal_config->num_ckvs * sizeof(agm_key_value);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_AIF_SET_CAL};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&audio_intf, sizeof(audio_intf));
    write(cal_config, sizeof(agm_cal_config) +
                          cal_config->num_ckvs * sizeof(agm_key_value));

    read(&rst, sizeof(rst));
    return rst;
}

int agm_session_set_params(uint32_t session_id, void *payload, size_t size) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(size) + size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_SET_PARAMS};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&size, sizeof(size));
    write(payload, size);

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_get_params(uint32_t session_id, void *payload, size_t size) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(size);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_GET_PARAMS};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&size, sizeof(size));

    read(&rst, sizeof(rst));
    if (!rst && size)
        read(payload, size);

    return rst;
}

int agm_get_params_from_acdb_tunnel(void *payload, size_t *size) {
    int rst = 0;
    size_t payloadSize = sizeof(size);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_GET_PARAMS_FROM_ACDB_TUNNEL};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(size, sizeof(size));

    read(&rst, sizeof(rst));
    if (!rst) {
        read(size, sizeof(size_t));
        read(payload, *size);
    }
    return rst;
}

int agm_set_params_with_tag(uint32_t session_id, uint32_t aif_id,
                            struct agm_tag_config *tag_config) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(aif_id) +
                         sizeof(agm_tag_config) +
                         tag_config->num_tkvs * sizeof(agm_key_value);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SET_PARAMS_WITH_TAG};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&aif_id, sizeof(aif_id));
    write(tag_config, sizeof(agm_tag_config) +
                          tag_config->num_tkvs * sizeof(agm_key_value));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_set_params_with_tag_to_acdb(uint32_t session_id, uint32_t aif_id,
                                    void *payload, size_t size) {
    int rst = 0;
    size_t payloadSize =
        sizeof(session_id) + sizeof(aif_id) + sizeof(size) + size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SET_PARAMS_WITH_TAG_TO_ACDB};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&aif_id, sizeof(aif_id));
    write(&size, sizeof(size));
    write(payload, size);

    read(&rst, sizeof(rst));

    return rst;
}

int agm_set_params_to_acdb_tunnel(void *payload, size_t size) {
    int rst = 0;
    size_t payloadSize = sizeof(size) + size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SET_PARAMS_TO_ACDB_TUNNEL};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&size, sizeof(size));
    write(payload, size);

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_register_cb(uint32_t session_id, agm_event_cb cb,
                            enum event_type event, void *client_data) {
    int rst = 0;
    size_t payloadSize =
        sizeof(session_id) + sizeof(cb) + sizeof(event) + sizeof(client_data);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_REGISTER_CB};
    AGM_LOGI("%s called session_id %d cb %p event %d client_data %p \n",
             __func__, session_id, cb, event, client_data);
    get_agm_server_socket();

    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&cb, sizeof(cb));
    write(&event, sizeof(event));
    write(&client_data, sizeof(client_data));

    read(&rst, sizeof(rst));
    return rst;
}

int agm_session_register_for_events(uint32_t session_id,
                                    struct agm_event_reg_cfg *reg_cfg) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(agm_event_reg_cfg) +
                         reg_cfg->event_config_payload_size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_REGISTER_FOR_EVENTS};
    AGM_LOGI("invoke %s", __func__);
    get_agm_server_socket();

    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(reg_cfg,
          sizeof(agm_event_reg_cfg) + reg_cfg->event_config_payload_size);

    read(&rst, sizeof(rst));
    return rst;
}

int agm_session_open(uint32_t session_id, enum agm_session_mode sess_mode,
                     uint64_t *handle) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(sess_mode);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_OPEN};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&sess_mode, sizeof(sess_mode));

    read(&rst, sizeof(rst));
    if (!rst) {
        read(handle, sizeof(handle));
    }

    return rst;
}

int agm_session_set_config(uint64_t handle,
                           struct agm_session_config *session_config,
                           struct agm_media_config *media_config,
                           struct agm_buffer_config *buffer_config) {
    int rst = 0;
    size_t payloadSize = sizeof(handle) + sizeof(agm_session_config) +
                         sizeof(agm_media_config) + sizeof(agm_buffer_config);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_SET_CONFIG};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));
    write(session_config, sizeof(agm_session_config));
    write(media_config, sizeof(agm_media_config));
    write(buffer_config, sizeof(agm_buffer_config));

    read(&rst, sizeof(rst));
    return rst;
}

int agm_session_close(uint64_t handle) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_CLOSE};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_prepare(uint64_t handle) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_PREPARE};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_start(uint64_t handle) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_START};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_stop(uint64_t handle) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_STOP};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_pause(uint64_t handle) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_PAUSE};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_flush(uint64_t handle) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_FLUSH};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_sessionid_flush(uint32_t session_id) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSIONID_FLUSH};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_resume(uint64_t handle) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_RESUME};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_suspend(uint64_t handle) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_SUSPEND};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_read(uint64_t handle, void *buff, size_t *count) {
    int rst = 0;
    size_t payloadSize = sizeof(handle) + sizeof(size_t);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_READ};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));
    write(count, sizeof(size_t));

    read(&rst, sizeof(rst));

    if (!rst) {
        read(count, sizeof(size_t));
        if (*count > 0)
            read(buff, *count);
    }
    return rst;
}

int agm_session_write(uint64_t handle, void *buf, size_t *count) {
    int rst = 0;
    size_t payloadSize = sizeof(handle) + sizeof(size_t) + *count;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_WRITE};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));
    write(count, sizeof(size_t));
    write(buf, *count);

    read(&rst, sizeof(rst));
    if (!rst) {
        read(count, sizeof(size_t));
    }

    return rst;
}

size_t agm_get_hw_processed_buff_cnt(uint64_t handle, enum direction dir) {
    int cnt = 0;
    size_t payloadSize = sizeof(handle) + sizeof(dir);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_GET_HW_PROCESSED_BUFF_CNT};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));
    write(&dir, sizeof(direction));

    read(&cnt, sizeof(cnt));
    return cnt;
}

int agm_get_aif_info_list(struct aif_info *aif_list, size_t *num_aif_info) {
    int rst = 0;
    size_t payloadSize = sizeof(size_t);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_GET_AIF_INFO_LIST};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(num_aif_info, sizeof(size_t));

    read(&rst, sizeof(rst));
    read(num_aif_info, sizeof(size_t));
    if (!rst && *num_aif_info && aif_list)
        read(aif_list, *num_aif_info * sizeof(aif_info));

    return rst;
}

int agm_get_non_alsa_aif_info_list(struct non_alsa_aif_info *aif_list, size_t *num_aif_info) {
    int rst = 0;
    num_aif_info = 0;
    return rst;
}

int agm_session_set_loopback(uint32_t capture_session_id,
                             uint32_t playback_session_id, bool state) {
    int rst = 0;
    size_t payloadSize = sizeof(capture_session_id) +
                         sizeof(playback_session_id) + sizeof(state);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_SET_LOOPBACK};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&capture_session_id, sizeof(capture_session_id));
    write(&playback_session_id, sizeof(playback_session_id));
    write(&state, sizeof(state));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_set_ec_ref(uint32_t capture_session_id, uint32_t aif_id,
                           bool state) {
    int rst = 0;
    size_t payloadSize =
        sizeof(capture_session_id) + sizeof(aif_id) + sizeof(state);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_SET_EC_REF};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&capture_session_id, sizeof(capture_session_id));
    write(&aif_id, sizeof(aif_id));
    write(&state, sizeof(state));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_eos(uint64_t handle) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_EOS};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_get_session_time(uint64_t handle, uint64_t *timestamp) {
    int rst = 0;
    size_t payloadSize = sizeof(handle);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_GET_SESSION_TIME};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));

    read(&rst, sizeof(rst));

    if (!rst) {
        read(timestamp, sizeof(uint64_t));
    }

    return rst;
}

int agm_get_buffer_timestamp(uint32_t session_id, uint64_t *timestamp) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_GET_BUFFER_TIMESTAMP};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));

    read(&rst, sizeof(rst));

    if (!rst) {
        read(timestamp, sizeof(uint64_t));
    }

    return rst;
}

int agm_session_get_buf_info(uint32_t session_id, struct agm_buf_info *buf_info,
                             uint32_t flag) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(flag);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_GET_BUF_INFO};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(&flag, sizeof(flag));

    read(&rst, sizeof(rst));

    if (!rst) {
        read(buf_info, sizeof(agm_buf_info));
    }

    return rst;
}

int agm_register_service_crash_callback(agm_service_crash_cb cb,
                                        uint64_t cookie) {
    int rst = 0;
    size_t payloadSize = sizeof(cb) + sizeof(cookie);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_REGISTER_SERVICE_CRASH_CALLBACK};
    AGM_LOGI("%s called \n", __func__);
    get_agm_server_socket();

    write(&header, sizeof(header));
    write(&cb, sizeof(cb));
    write(&cookie, sizeof(cookie));

    read(&rst, sizeof(rst));
    return rst;
}

int agm_set_gapless_session_metadata(uint64_t handle,
                                     enum agm_gapless_silence_type type,
                                     uint32_t silence) {
    int rst = 0;
    size_t payloadSize = sizeof(handle) + sizeof(type) + sizeof(silence);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SET_GAPLESS_SESSION_METADATA};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));
    write(&type, sizeof(type));
    write(&silence, sizeof(silence));

    read(&rst, sizeof(rst));

    return rst;
}

int agm_session_write_with_metadata(uint64_t handle, struct agm_buff *buff,
                                    size_t *consumed_size) {
    int rst = 0;
    size_t payloadSize = sizeof(handle) + sizeof(struct agm_buff) + buff->size +
                         buff->metadata_size + sizeof(size_t);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_WRITE_WITH_METADATA};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));
    write(buff, sizeof(struct agm_buff));
    if (buff->size > 0) {
        write(buff->addr, buff->size);
    }
    if (buff->metadata_size > 0) {
        write(buff->metadata, buff->metadata_size);
    }
    write(consumed_size, sizeof(size_t));

    read(&rst, sizeof(rst));
    if (!rst)
        read(consumed_size, sizeof(size_t));
    return rst;
}

int agm_session_read_with_metadata(uint64_t handle, struct agm_buff *buff,
                                   uint32_t *captured_size) {
    int rst = 0;
    size_t payloadSize =
        sizeof(handle) + sizeof(struct agm_buff) + sizeof(size_t);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_READ_WITH_METADATA};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));
    write(buff, sizeof(struct agm_buff));
    write(captured_size, sizeof(size_t));

    read(&rst, sizeof(rst));
    if (!rst) {
        read(buff, sizeof(agm_buff));
        if (buff->size)
            read(buff->addr, buff->size);
        if (buff->metadata_size)
            read(buff->metadata, buff->metadata_size);
        read(captured_size, sizeof(size_t));
    }
    return rst;
}

int agm_session_set_non_tunnel_mode_config(
    uint64_t handle, struct agm_session_config *session_config,
    struct agm_media_config *in_media_config,
    struct agm_media_config *out_media_config,
    struct agm_buffer_config *in_buffer_config,
    struct agm_buffer_config *out_buffer_config) {
    int rst = 0;
    size_t payloadSize =
        sizeof(handle) + sizeof(agm_session_config) +
        2 * (sizeof(agm_media_config) + sizeof(agm_buffer_config));
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_SET_NON_TUNNEL_MODE_CONFIG};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&handle, sizeof(handle));
    write(session_config, sizeof(agm_session_config));
    write(in_media_config, sizeof(agm_media_config));
    write(out_media_config, sizeof(agm_media_config));
    write(in_buffer_config, sizeof(agm_buffer_config));
    write(out_buffer_config, sizeof(agm_buffer_config));

    read(&rst, sizeof(rst));
    return rst;
}

int agm_get_group_aif_info_list(struct aif_info *aif_list, size_t *num_groups) {
    int rst = 0;
    size_t payloadSize = sizeof(size_t);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_GET_GROUP_AIF_INFO_LIST};
    AGM_LOGI("invoke %s", __func__);
    get_agm_server_socket();
    write(&header, sizeof(header));
    write(num_groups, sizeof(size_t));

    read(&rst, sizeof(rst));
    read(num_groups, sizeof(size_t));
    if (!rst && *num_groups && aif_list)
        read(aif_list, *num_groups * sizeof(aif_info));

    return rst;
}

int agm_aif_group_set_media_config(
    uint32_t aif_group_id, struct agm_group_media_config *media_config) {
    int rst = 0;
    size_t payloadSize = sizeof(aif_group_id) + sizeof(agm_group_media_config);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_AIF_GROUP_SET_MEDIA_CONFIG};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&aif_group_id, sizeof(aif_group_id));
    write(media_config, sizeof(agm_group_media_config));

    read(&rst, sizeof(rst));
    return rst;
}

int agm_session_write_datapath_params(uint32_t session_id,
                                      struct agm_buff *buff) {
    int rst = 0;
    size_t payloadSize = sizeof(session_id) + sizeof(struct agm_buff) +
                         buff->size + buff->metadata_size;
    IPCHeader header = {sizeof(IPCHeader) + payloadSize,
                        Opcode::AGM_SESSION_WRITE_DATAPATH_PARAMS};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(&session_id, sizeof(session_id));
    write(buff, sizeof(struct agm_buff));
    if (buff->size > 0) {
        write(buff->addr, buff->size);
    }
    if (buff->metadata_size > 0) {
        write(buff->metadata, buff->metadata_size);
    }

    read(&rst, sizeof(rst));
    return rst;
}

int agm_dump(struct agm_dump_info *dump_info) {
    int rst = 0;
    size_t payloadSize = sizeof(agm_dump_info);
    IPCHeader header = {sizeof(IPCHeader) + payloadSize, Opcode::AGM_DUMP};
    AGM_LOGI("invoke %s", __func__);

    get_agm_server_socket();
    write(&header, sizeof(header));
    write(dump_info, sizeof(agm_dump_info));

    read(&rst, sizeof(rst));
    return rst;
}
