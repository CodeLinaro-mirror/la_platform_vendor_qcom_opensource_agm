/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.  */
/* SPDX-License-Identifier: BSD-3-Clause-Clear */

#define LOG_TAG "agm_ipc_ls_be"

#include <utils/Log.h>

#include "agm_ipc_ls_be.h"
#include "ipc_common.h"
#include "utils.h"
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_SERVER_WRAPPER
#include <log_utils.h>
#endif

int write_check(int socket, void *buff, size_t size) {
    int len = 0;
    len = write(socket, buff, size);
    if (len < 0) {
        AGM_LOGE("IPC socket write Error: %s (errno=%d)\n", strerror(errno),
                 errno);
    }
    return len;
}

int read_check(int socket, void *buff, size_t size) {
    int len = 0;
    len = recv(socket, buff, size, MSG_WAITALL);
    if (len < 0) {
        AGM_LOGE("IPC socket recv Error: %s (errno=%d)\n", strerror(errno),
                 errno);
    }
    return len;
}

AgmIpcLsBackend::~AgmIpcLsBackend() { stop(); }

void AgmIpcLsBackend::start() {
    sockaddr_un backendAddr = {};
    int frontendSocket = 0;
    sockaddr_un cbBackendAddr = {};
    int cbFrontEndSocket = 0;

    // Create a UNIX domain socket
    backendSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (backendSocket == -1) {
        AGM_LOGE("Failed to create Agm Back end socket");
        goto end;
    }
    cbBackendSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cbBackendSocket == -1) {
        AGM_LOGE("Failed to create Agm Callback Back end socket");
        goto end;
    }

    memset(&backendAddr, 0, sizeof(backendAddr));
    backendAddr.sun_family = AF_UNIX;
    strlcpy(backendAddr.sun_path, AGM_SOCKET_PATH,
            sizeof(backendAddr.sun_path) - 1);

    unlink(AGM_SOCKET_PATH);

    // Bind the socket to the file path
    if (bind(backendSocket, (sockaddr *)&backendAddr, sizeof(backendAddr)) ==
        -1) {
        close(backendSocket);
        AGM_LOGE("Failed to bind Agm Back end socket");
        goto end;
    }

    memset(&cbBackendAddr, 0, sizeof(cbBackendAddr));
    cbBackendAddr.sun_family = AF_UNIX;
    strlcpy(cbBackendAddr.sun_path, AGM_CB_SOCKET_PATH,
            sizeof(cbBackendAddr.sun_path) - 1);

    unlink(AGM_CB_SOCKET_PATH);

    if (bind(cbBackendSocket, (sockaddr *)&cbBackendAddr,
             sizeof(cbBackendAddr)) == -1) {
        close(cbBackendSocket);
        AGM_LOGE("Failed to bind Agm Callback Back end socket");
        goto end;
    }

    // Start listening for connections
    if (listen(backendSocket, 5) == -1) {
        close(backendSocket);
        AGM_LOGE("Failed to listen on socket");
        goto end;
    }
    if (listen(cbBackendSocket, 5) == -1) {
        close(cbBackendSocket);
        AGM_LOGE("Failed to listen on callback socket");
        goto end;
    }

    AGM_LOGI("IPC Back end listening on %s", AGM_SOCKET_PATH);
    AGM_LOGI("IPC Callback Back end listening on %s", AGM_CB_SOCKET_PATH);

    while (true) {
        frontendSocket = accept(backendSocket, nullptr, nullptr);
        if (frontendSocket == -1) {
            AGM_LOGE("Failed to accept Agm Back end socket");
            continue;
        }
        cbFrontEndSocket = accept(cbBackendSocket, nullptr, nullptr);
        if (cbFrontEndSocket == -1) {
            AGM_LOGE("Failed to accept Agm callback Back end socket");
            continue;
        }

        AGM_LOGI("New client connected");

        std::thread(&AgmIpcLsBackend::handleClient, this, frontendSocket,
                    cbFrontEndSocket)
            .detach();
    }
end:
    return;
}

void AgmIpcLsBackend::stop() {
    if (backendSocket != -1) {
        close(backendSocket);
        unlink(AGM_SOCKET_PATH);
        backendSocket = -1;
    }
    if (cbBackendSocket != -1) {
        close(cbBackendSocket);
        unlink(AGM_CB_SOCKET_PATH);
        cbBackendSocket = -1;
    }
}

void AgmIpcLsBackend::handleClient(int frontendSocket, int cbFrontEndSocket) {
    IPCHeader header = {};
    ssize_t bytesRead = 0;
    char *payload = nullptr;
    int payloadSize = 0;

    while (true) {
        bytesRead = read_check(frontendSocket, &header, sizeof(header));
        if (bytesRead <= 0) {
            break; // Client disconnected
        }

        payloadSize = header.totalLength - sizeof(header);
        if (payloadSize > 0) {
            payload = (char *)malloc(payloadSize);
            if (!payload) {
                AGM_LOGE("malloc failed");
                goto end;
            }
            bytesRead = read_check(frontendSocket, payload, payloadSize);
            if (bytesRead <= 0) {
                goto end;
            }
        }
        AGM_LOGI("recieve opcode %d payloadSize %d", header.opcode,
                 payloadSize);

        switch (header.opcode) {
        case Opcode::AGM_AIF_SET_MEDIA_CONFIG:
            ipc_agm_aif_set_media_config(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_AIF_SET_METADATA:
            ipc_agm_aif_set_metadata(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_SET_METADATA:
            ipc_agm_session_set_metadata(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_AIF_SET_METADATA:
            ipc_agm_session_aif_set_metadata(frontendSocket, payload,
                                             payloadSize);
            break;
        case Opcode::AGM_SESSION_AIF_CONNECT:
            ipc_agm_session_aif_connect(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_AIF_GET_TAG_MODULE_INFO:
            ipc_agm_session_aif_get_tag_module_info(frontendSocket, payload,
                                                    payloadSize);
            break;
        case Opcode::AGM_AIF_SET_PARAMS:
            ipc_agm_aif_set_params(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_AIF_SET_PARAMS:
            ipc_agm_session_aif_set_params(frontendSocket, payload,
                                           payloadSize);
            break;
        case Opcode::AGM_SESSION_AIF_SET_CAL:
            ipc_agm_session_aif_set_cal(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_SET_PARAMS:
            ipc_agm_session_set_params(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_GET_PARAMS:
            ipc_agm_session_get_params(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_GET_PARAMS_FROM_ACDB_TUNNEL:
            ipc_agm_get_params_from_acdb_tunnel(frontendSocket, payload,
                                                payloadSize);
            break;
        case Opcode::AGM_SET_PARAMS_WITH_TAG:
            ipc_agm_set_params_with_tag(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SET_PARAMS_WITH_TAG_TO_ACDB:
            ipc_agm_set_params_with_tag_to_acdb(frontendSocket, payload,
                                                payloadSize);
            break;
        case Opcode::AGM_SET_PARAMS_TO_ACDB_TUNNEL:
            ipc_agm_set_params_to_acdb_tunnel(frontendSocket, payload,
                                              payloadSize);
            break;
        case Opcode::AGM_SESSION_REGISTER_CB:
            ipc_agm_session_register_cb(frontendSocket, cbFrontEndSocket,
                                        payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_REGISTER_FOR_EVENTS:
            ipc_agm_session_register_for_events(frontendSocket, payload,
                                                payloadSize);
            break;
        case Opcode::AGM_SESSION_OPEN:
            ipc_agm_session_open(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_SET_CONFIG:
            ipc_agm_session_set_config(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_CLOSE:
            ipc_agm_session_close(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_PREPARE:
            ipc_agm_session_prepare(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_START:
            ipc_agm_session_start(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_STOP:
            ipc_agm_session_stop(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_PAUSE:
            ipc_agm_session_pause(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_FLUSH:
            ipc_agm_session_flush(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSIONID_FLUSH:
            ipc_agm_sessionid_flush(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_RESUME:
            ipc_agm_session_resume(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_SUSPEND:
            ipc_agm_session_suspend(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_READ:
            ipc_agm_session_read(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_WRITE:
            ipc_agm_session_write(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_GET_HW_PROCESSED_BUFF_CNT:
            ipc_agm_get_hw_processed_buff_cnt(frontendSocket, payload,
                                              payloadSize);
            break;
        case Opcode::AGM_GET_AIF_INFO_LIST:
            ipc_agm_get_aif_info_list(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_SET_LOOPBACK:
            ipc_agm_session_set_loopback(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_SET_EC_REF:
            ipc_agm_session_set_ec_ref(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_EOS:
            ipc_agm_session_eos(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_GET_SESSION_TIME:
            ipc_agm_get_session_time(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_GET_BUFFER_TIMESTAMP:
            ipc_agm_get_buffer_timestamp(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SESSION_GET_BUF_INFO:
            ipc_agm_session_get_buf_info(frontendSocket, payload, payloadSize);
            break;
        case Opcode::AGM_REGISTER_SERVICE_CRASH_CALLBACK:
            ipc_agm_register_service_crash_callback(
                frontendSocket, cbFrontEndSocket, payload, payloadSize);
            break;
        case Opcode::AGM_SET_GAPLESS_SESSION_METADATA:
            ipc_agm_set_gapless_session_metadata(frontendSocket, payload,
                                                 payloadSize);
            break;
        case Opcode::AGM_SESSION_WRITE_WITH_METADATA:
            ipc_agm_session_write_with_metadata(frontendSocket, payload,
                                                payloadSize);
            break;
        case Opcode::AGM_SESSION_READ_WITH_METADATA:
            ipc_agm_session_read_with_metadata(frontendSocket, payload,
                                               payloadSize);
            break;
        case Opcode::AGM_SESSION_SET_NON_TUNNEL_MODE_CONFIG:
            ipc_agm_session_set_non_tunnel_mode_config(frontendSocket, payload,
                                                       payloadSize);
            break;
        case Opcode::AGM_GET_GROUP_AIF_INFO_LIST:
            ipc_agm_get_group_aif_info_list(frontendSocket, payload,
                                            payloadSize);
            break;
        case Opcode::AGM_AIF_GROUP_SET_MEDIA_CONFIG:
            ipc_agm_aif_group_set_media_config(frontendSocket, payload,
                                               payloadSize);
            break;
        case Opcode::AGM_SESSION_WRITE_DATAPATH_PARAMS:
            ipc_agm_session_write_datapath_params(frontendSocket, payload,
                                                  payloadSize);
            break;
        case Opcode::AGM_DUMP:
            ipc_agm_dump(frontendSocket, payload, payloadSize);
            break;
        case Opcode::DISCONNECT:
            AGM_LOGI("Client requested disconnect");
            delete[] payload;
            close(frontendSocket);
            return;
        default:
            AGM_LOGI("Unknown opcode: %d", static_cast<int>(header.opcode));
            break;
        }
        free(payload);
        payload = nullptr;
    }
end:
    if (payload)
        free(payload);
    AGM_LOGI("Client disconnected");
    close(frontendSocket);
    close(cbFrontEndSocket);
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

void AgmIpcLsBackend::ipc_agm_aif_set_media_config(int frontendSocket,
                                                   const char *payload,
                                                   size_t payloadSize) {
    int rst = 0;
    uint32_t *aifId = nullptr;
    agm_media_config *mediaConfig = nullptr;
    const char *pEnd = payload + payloadSize;

    aifId = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    mediaConfig = (agm_media_config *)payload;
    payload += sizeof(agm_media_config);

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_aif_set_media_config(*aifId, mediaConfig);

    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_aif_set_metadata(int frontendSocket,
                                               const char *payload,
                                               size_t payloadSize) {
    int rst = 0;
    uint32_t *audio_intf = nullptr;
    uint32_t *size = nullptr;
    uint8_t *metadata = nullptr;
    const char *pEnd = payload + payloadSize;

    audio_intf = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    size = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    metadata = (uint8_t *)payload;
    payload += *size;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_aif_set_metadata(*audio_intf, *size, metadata);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_set_metadata(int frontendSocket,
                                                   const char *payload,
                                                   size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint32_t *size = nullptr;
    uint8_t *metadata = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    size = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    metadata = (uint8_t *)payload;
    payload += *size;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_set_metadata(*session_id, *size,
                                                      metadata);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_aif_set_metadata(int frontendSocket,
                                                       const char *payload,
                                                       size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint32_t *audio_intf = nullptr;
    uint32_t *size = nullptr;
    uint8_t *metadata = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    audio_intf = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    size = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    metadata = (uint8_t *)payload;
    payload += *size;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_aif_set_metadata(
        *session_id, *audio_intf, *size, metadata);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_aif_connect(int frontendSocket,
                                                  const char *payload,
                                                  size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint32_t *audio_intf = nullptr;
    bool *state = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    audio_intf = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    state = (bool *)payload;
    payload += sizeof(bool);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_aif_connect(*session_id, *audio_intf,
                                                     *state);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_aif_get_tag_module_info(
    int frontendSocket, const char *payload, size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint32_t *aif_id = nullptr;
    size_t size = 0;
    char *info = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    aif_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    memcpy(&size, payload, sizeof(size));
    payload += sizeof(size);

    if (size > 0) {
        info = (char *)malloc(size);
        if (!info) {
            AGM_LOGE("malloc failed");
            rst = -1;
            write_check(frontendSocket, &rst, sizeof(rst));
            goto end;
        }
    }

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_aif_get_tag_module_info(
        *session_id, *aif_id, info, &size);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst) {
        write_check(frontendSocket, &size, sizeof(size));
        if (info) {
            write_check(frontendSocket, info, size);
        }
    }
end:
    if (info)
        free(info);
}

void AgmIpcLsBackend::ipc_agm_aif_set_params(int frontendSocket,
                                             const char *payload,
                                             size_t payloadSize) {
    int rst = 0;
    uint32_t *aif_id = nullptr;
    uint32_t *size = nullptr;
    void *param = nullptr;
    const char *pEnd = payload + payloadSize;

    aif_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    size = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    param = (void *)payload;
    payload += *size;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_aif_set_params(*aif_id, param, *size);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_aif_set_params(int frontendSocket,
                                                     const char *payload,
                                                     size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint32_t *aif_id = nullptr;
    size_t *size = nullptr;
    void *param = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    aif_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    size = (size_t *)payload;
    payload += sizeof(size_t);
    param = (void *)payload;
    payload += *size;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_aif_set_params(*session_id, *aif_id,
                                                        param, *size);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_aif_set_cal(int frontendSocket,
                                                  const char *payload,
                                                  size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint32_t *audio_intf = nullptr;
    agm_cal_config *config = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    audio_intf = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    config = (agm_cal_config *)payload;
    payload +=
        sizeof(agm_cal_config) + config->num_ckvs * sizeof(agm_key_value);

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_aif_set_cal(*session_id, *audio_intf,
                                                     config);

    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_set_params(int frontendSocket,
                                                 const char *payload,
                                                 size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    size_t *size = nullptr;
    void *param = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    size = (size_t *)payload;
    payload += sizeof(size_t);
    param = (void *)payload;
    payload += *size;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_set_params(*session_id, param, *size);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_get_params(int frontendSocket,
                                                 const char *payload,
                                                 size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    size_t *size = nullptr;
    void *params = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    size = (size_t *)payload;
    payload += sizeof(size_t);

    if (*size) {
        params = (void *)malloc(*size);
        if (!params) {
            AGM_LOGE("malloc failed");
            rst = -1;
            write_check(frontendSocket, &rst, sizeof(rst));
            goto end;
        }
    }

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_get_params(*session_id, params, *size);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst) {
        write_check(frontendSocket, params, *size);
    }
end:
    if (params)
        free(params);
}

void AgmIpcLsBackend::ipc_agm_get_params_from_acdb_tunnel(int frontendSocket,
                                                          const char *payload,
                                                          size_t payloadSize) {
    int rst = 0;
    size_t *size = nullptr;
    void *params = nullptr;
    const char *pEnd = payload + payloadSize;

    size = (size_t *)payload;
    payload += sizeof(size_t);

    if (*size) {
        params = (void *)malloc(*size);
        if (!params) {
            AGM_LOGE("malloc failed");
            rst = -1;
            write_check(frontendSocket, &rst, sizeof(rst));
            goto end;
        }
    }

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_get_params_from_acdb_tunnel(params, size);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst) {
        write_check(frontendSocket, size, sizeof(size_t));
        write_check(frontendSocket, params, *size);
    }
end:
    if (params)
        free(params);
}

void AgmIpcLsBackend::ipc_agm_set_params_with_tag(int frontendSocket,
                                                  const char *payload,
                                                  size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint32_t *aif_id = nullptr;
    agm_tag_config *config = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    aif_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    config = (agm_tag_config *)payload;
    payload +=
        sizeof(agm_tag_config) + config->num_tkvs * sizeof(agm_key_value);

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_set_params_with_tag(*session_id, *aif_id,
                                                     config);

    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_set_params_with_tag_to_acdb(int frontendSocket,
                                                          const char *payload,
                                                          size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint32_t *aif_id = nullptr;
    size_t *size = nullptr;
    void *param = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    aif_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    size = (size_t *)payload;
    payload += sizeof(size_t);
    param = (void *)payload;
    payload += *size;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_set_params_with_tag_to_acdb(
        *session_id, *aif_id, param, *size);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_set_params_to_acdb_tunnel(int frontendSocket,
                                                        const char *payload,
                                                        size_t payloadSize) {
    int rst = 0;
    size_t *size = nullptr;
    void *param = nullptr;
    const char *pEnd = payload + payloadSize;

    size = (size_t *)payload;
    payload += sizeof(size_t);
    param = (void *)payload;
    payload += *size;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_set_params_to_acdb_tunnel(param, *size);
    write_check(frontendSocket, &rst, sizeof(rst));
}

struct agm_session_event_clbk_client_data {
    int cbFrontEndSocket;
    uint32_t session_id;
    agm_event_cb cb;
    event_type type;
    void *client_data;
};

struct agm_crash_clbk_client_data {
    int cbFrontEndSocket;
    agm_service_crash_cb cb;
    uint64_t cookie;
};

enum agm_clbk_data_type { TYPE_EVENT, TYPE_CRASH };

struct agm_clbk_data {
    agm_clbk_data_type type;
    union {
        agm_session_event_clbk_client_data *event_cb_data;
        agm_crash_clbk_client_data *crash_cb_data;
    };
};

static pthread_mutex_t clbk_data_list_lock = PTHREAD_MUTEX_INITIALIZER;
static std::vector<agm_clbk_data> agm_clbk_data_list;

void ipc_agm_event_cb(uint32_t session_id, agm_event_cb_params *evt_param,
                      void *client_data) {
    agm_session_event_clbk_client_data *cdata =
        (agm_session_event_clbk_client_data *)client_data;
    pthread_mutex_lock(&clbk_data_list_lock);
    int cbFrontEndSocket = cdata->cbFrontEndSocket;
    agm_event_cb cb = cdata->cb;
    client_data = cdata->client_data;
    pthread_mutex_unlock(&clbk_data_list_lock);
    size_t payloadSize = sizeof(cb) + sizeof(session_id) +
                         sizeof(agm_event_cb_params) +
                         evt_param->event_payload_size + sizeof(client_data);
    IPCCbHeader header = {sizeof(IPCCbHeader) + payloadSize,
                          Cbcode::AGM_EVETN_CB};
    write_check(cbFrontEndSocket, &header, sizeof(header));
    write_check(cbFrontEndSocket, &cb, sizeof(cb));
    write_check(cbFrontEndSocket, &session_id, sizeof(session_id));
    write_check(cbFrontEndSocket, evt_param,
                sizeof(agm_event_cb_params) + evt_param->event_payload_size);
    write_check(cbFrontEndSocket, &client_data, sizeof(client_data));
}

void AgmIpcLsBackend::ipc_agm_session_register_cb(int frontendSocket,
                                                  int cbFrontEndSocket,
                                                  const char *payload,
                                                  size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    agm_event_cb *client_cb = nullptr;
    agm_event_cb cb = nullptr;
    event_type *type = nullptr;
    void **client_data = nullptr;
    agm_clbk_data cdata = {.type = TYPE_EVENT, .event_cb_data = nullptr};
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload, payload += sizeof(uint32_t);
    client_cb = (agm_event_cb *)payload, payload += sizeof(agm_event_cb);
    type = (event_type *)payload, payload += sizeof(event_type);
    client_data = (void **)payload, payload += sizeof(void *);

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    if (*client_cb) {
        cb = ipc_agm_event_cb;
        cdata.event_cb_data = (agm_session_event_clbk_client_data *)malloc(
            sizeof(agm_session_event_clbk_client_data));
        if (!cdata.event_cb_data) {
            AGM_LOGE("malloc failed");
            rst = -1;
            goto end;
        }
        cdata.event_cb_data->cbFrontEndSocket = cbFrontEndSocket;
        cdata.event_cb_data->session_id = *session_id;
        cdata.event_cb_data->cb = *client_cb;
        cdata.event_cb_data->type = *type;
        cdata.event_cb_data->client_data = *client_data;
        pthread_mutex_lock(&clbk_data_list_lock);
        agm_clbk_data_list.emplace_back(cdata);
        pthread_mutex_unlock(&clbk_data_list_lock);
        rst = agmLsBeWrapper.ipc_agm_session_register_cb(*session_id, cb, *type,
                                                         cdata.event_cb_data);
        AGM_LOGV("ipc_agm_session_register_cb session id %d malloc 0x%p",
                 *session_id, *client_cb);
    } else {
        cb = nullptr;
        pthread_mutex_lock(&clbk_data_list_lock);
        auto item = agm_clbk_data_list.begin();
        while (item != agm_clbk_data_list.end()) {
            if (item->type == TYPE_EVENT &&
                item->event_cb_data->session_id == *session_id &&
                item->event_cb_data->type == *type &&
                item->event_cb_data->client_data == *client_data) {
                rst = agmLsBeWrapper.ipc_agm_session_register_cb(
                    *session_id, cb, *type, item->event_cb_data);
                AGM_LOGV("ipc_agm_session_register_cb session id %d free 0x%p",
                         *session_id, *client_cb);
                free(item->event_cb_data);
                item = agm_clbk_data_list.erase(item);
            } else {
                ++item;
            }
        }
        pthread_mutex_unlock(&clbk_data_list_lock);
    }
end:
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_register_for_events(int frontendSocket,
                                                          const char *payload,
                                                          size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    agm_event_reg_cfg *reg_cfg = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload, payload += sizeof(uint32_t);
    reg_cfg = (agm_event_reg_cfg *)payload;
    payload += sizeof(agm_event_reg_cfg) + reg_cfg->event_config_payload_size;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_register_for_events(*session_id,
                                                             reg_cfg);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_open(int frontendSocket,
                                           const char *payload,
                                           size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    enum agm_session_mode *sess_mode = nullptr;
    uint64_t handle = 0;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload, payload += sizeof(uint32_t);
    sess_mode = (agm_session_mode *)payload,
    payload += sizeof(agm_session_mode);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_open(*session_id, *sess_mode, &handle);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst) {
        write_check(frontendSocket, &handle, sizeof(handle));
    }
}

void AgmIpcLsBackend::ipc_agm_session_set_config(int frontendSocket,
                                                 const char *payload,
                                                 size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    agm_session_config *session_config = nullptr;
    agm_media_config *media_config = nullptr;
    agm_buffer_config *buffer_config = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    session_config = (agm_session_config *)payload;
    payload += sizeof(agm_session_config);
    media_config = (agm_media_config *)payload;
    payload += sizeof(agm_media_config);
    buffer_config = (agm_buffer_config *)payload;
    payload += sizeof(agm_buffer_config);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_set_config(
        *handle, session_config, media_config, buffer_config);

    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_close(int frontendSocket,
                                            const char *payload,
                                            size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_close(*handle);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_prepare(int frontendSocket,
                                              const char *payload,
                                              size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_prepare(*handle);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_start(int frontendSocket,
                                            const char *payload,
                                            size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_start(*handle);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_stop(int frontendSocket,
                                           const char *payload,
                                           size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_stop(*handle);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_pause(int frontendSocket,
                                            const char *payload,
                                            size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_pause(*handle);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_flush(int frontendSocket,
                                            const char *payload,
                                            size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = 0;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_flush(*handle);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_sessionid_flush(int frontendSocket,
                                              const char *payload,
                                              size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_sessionid_flush(*session_id);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_resume(int frontendSocket,
                                             const char *payload,
                                             size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_resume(*handle);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_suspend(int frontendSocket,
                                              const char *payload,
                                              size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_suspend(*handle);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_read(int frontendSocket,
                                           const char *payload,
                                           size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    size_t count = 0;
    void *buffer = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    memcpy(&count, payload, sizeof(count));
    payload += sizeof(count);

    buffer = malloc(count);
    if (!buffer) {
        AGM_LOGE("malloc failed");
        rst = -1;
        write_check(frontendSocket, &rst, sizeof(rst));
        goto end;
    }

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_read(*handle, buffer, &count);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst) {
        write_check(frontendSocket, &count, sizeof(count));
        if (count > 0)
            write_check(frontendSocket, buffer, count);
    }

end:
    if (buffer)
        free(buffer);
}

void AgmIpcLsBackend::ipc_agm_session_write(int frontendSocket,
                                            const char *payload,
                                            size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    void *buf = nullptr;
    size_t count = 0;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    memcpy(&count, payload, sizeof(count));
    payload += sizeof(count);
    buf = (void *)payload;
    payload += count;
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_write(*handle, buf, &count);
    write_check(frontendSocket, &rst, sizeof(rst));

    if (!rst)
        write_check(frontendSocket, &count, sizeof(count));
}

void AgmIpcLsBackend::ipc_agm_get_hw_processed_buff_cnt(int frontendSocket,
                                                        const char *payload,
                                                        size_t payloadSize) {
    int cnt = 0;
    uint64_t *handle = nullptr;
    direction *dir = 0;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    dir = (direction *)payload;
    payload += sizeof(direction);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    cnt = agmLsBeWrapper.ipc_agm_get_hw_processed_buff_cnt(*handle, *dir);
    write_check(frontendSocket, &cnt, sizeof(cnt));
}

void AgmIpcLsBackend::ipc_agm_get_aif_info_list(int frontendSocket,
                                                const char *payload,
                                                size_t payloadSize) {
    int rst = 0;
    aif_info *aif_list = nullptr;
    size_t num_aif_info = 0;
    const char *pEnd = payload + payloadSize;

    memcpy(&num_aif_info, payload, sizeof(num_aif_info));
    payload += sizeof(num_aif_info);

    if (num_aif_info > 0) {
        aif_list = (aif_info *)malloc(num_aif_info * sizeof(aif_info));
        if (!aif_list) {
            AGM_LOGE("malloc failed");
            rst = -1;
            write_check(frontendSocket, &rst, sizeof(rst));
            goto end;
        }
    }

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_get_aif_info_list(aif_list, &num_aif_info);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst) {
        write_check(frontendSocket, &num_aif_info, sizeof(num_aif_info));
        if (aif_list) {
            write_check(frontendSocket, aif_list,
                        num_aif_info * sizeof(aif_info));
        }
    }
end:
    if (aif_list)
        free(aif_list);
}

void AgmIpcLsBackend::ipc_agm_session_set_loopback(int frontendSocket,
                                                   const char *payload,
                                                   size_t payloadSize) {
    int rst = 0;
    uint32_t *capture_session_id = nullptr;
    uint32_t *playback_session_id = nullptr;
    bool *state = nullptr;
    const char *pEnd = payload + payloadSize;

    capture_session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    playback_session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    state = (bool *)payload;
    payload += sizeof(bool);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_set_loopback(
        *capture_session_id, *playback_session_id, *state);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_set_ec_ref(int frontendSocket,
                                                 const char *payload,
                                                 size_t payloadSize) {
    int rst = 0;
    uint32_t *capture_session_id = nullptr;
    uint32_t *aif_id = nullptr;
    bool *state = nullptr;
    const char *pEnd = payload + payloadSize;

    capture_session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    aif_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    state = (bool *)payload;
    payload += sizeof(bool);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_set_ec_ref(*capture_session_id,
                                                    *aif_id, *state);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_eos(int frontendSocket,
                                          const char *payload,
                                          size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_eos(*handle);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_get_session_time(int frontendSocket,
                                               const char *payload,
                                               size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    uint64_t timestamp = 0;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_get_session_time(*handle, &timestamp);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst)
        write_check(frontendSocket, &timestamp, sizeof(timestamp));
}

void AgmIpcLsBackend::ipc_agm_get_buffer_timestamp(int frontendSocket,
                                                   const char *payload,
                                                   size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint64_t timestamp = 0;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_get_buffer_timestamp(*session_id, &timestamp);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst)
        write_check(frontendSocket, &timestamp, sizeof(timestamp));
}

void AgmIpcLsBackend::ipc_agm_session_get_buf_info(int frontendSocket,
                                                   const char *payload,
                                                   size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    uint32_t *flag = nullptr;
    agm_buf_info buf_info = {};
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    flag = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_get_buf_info(*session_id, &buf_info,
                                                      *flag);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst)
        write_check(frontendSocket, &buf_info, sizeof(buf_info));
}

void ipc_agm_service_crash_cb(uint64_t cookie) {
    agm_crash_clbk_client_data *cdata = (agm_crash_clbk_client_data *)cookie;
    pthread_mutex_lock(&clbk_data_list_lock);
    int cbFrontEndSocket = cdata->cbFrontEndSocket;
    agm_service_crash_cb cb = cdata->cb;
    cookie = cdata->cookie;
    pthread_mutex_unlock(&clbk_data_list_lock);
    size_t payloadSize = sizeof(cb) + sizeof(cookie);
    IPCCbHeader header = {sizeof(IPCCbHeader) + payloadSize,
                          Cbcode::AGM_SERVICE_CRASH_CB};
    write_check(cbFrontEndSocket, &header, sizeof(header));
    write_check(cbFrontEndSocket, &cb, sizeof(cb));
    write_check(cbFrontEndSocket, &cookie, sizeof(cookie));
}

void AgmIpcLsBackend::ipc_agm_register_service_crash_callback(
    int frontendSocket, int cbFrontEndSocket, const char *payload,
    size_t payloadSize) {
    int rst = 0;
    agm_service_crash_cb *client_cb = nullptr;
    agm_service_crash_cb cb = nullptr;
    uint64_t *cookie = nullptr;
    agm_clbk_data cdata = {.type = TYPE_CRASH, .crash_cb_data = nullptr};
    const char *pEnd = payload + payloadSize;

    client_cb = (agm_service_crash_cb *)payload,
    payload += sizeof(agm_service_crash_cb);
    cookie = (uint64_t *)payload, payload += sizeof(uint64_t);

    CHECK_PAYLOAD_ADDR(payload, pEnd);

    if (*client_cb) {
        cb = ipc_agm_service_crash_cb;
        cdata.crash_cb_data = (agm_crash_clbk_client_data *)malloc(
            sizeof(agm_crash_clbk_client_data));
        if (!cdata.crash_cb_data) {
            AGM_LOGE("malloc failed");
            rst = -1;
            goto end;
        }
        cdata.crash_cb_data->cbFrontEndSocket = cbFrontEndSocket;
        cdata.crash_cb_data->cb = *client_cb;
        cdata.crash_cb_data->cookie = *cookie;
        pthread_mutex_lock(&clbk_data_list_lock);
        agm_clbk_data_list.emplace_back(cdata);
        pthread_mutex_unlock(&clbk_data_list_lock);
        rst = agmLsBeWrapper.ipc_agm_register_service_crash_callback(
            cb, (uint64_t)cdata.crash_cb_data);
    } else {
        cb = nullptr;
        pthread_mutex_lock(&clbk_data_list_lock);
        auto item = agm_clbk_data_list.begin();
        while (item != agm_clbk_data_list.end()) {
            if (item->type == TYPE_CRASH &&
                item->crash_cb_data->cookie == *cookie) {
                rst = agmLsBeWrapper.ipc_agm_register_service_crash_callback(
                    cb, (uint64_t)item->crash_cb_data);
                free(item->crash_cb_data);
                item = agm_clbk_data_list.erase(item);
            } else {
                ++item;
            }
        }
        pthread_mutex_unlock(&clbk_data_list_lock);
    }
end:
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_set_gapless_session_metadata(int frontendSocket,
                                                           const char *payload,
                                                           size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    agm_gapless_silence_type *type = nullptr;
    uint32_t *silence = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    type = (agm_gapless_silence_type *)payload;
    payload += sizeof(agm_gapless_silence_type);
    silence = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_set_gapless_session_metadata(*handle, *type,
                                                              *silence);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_write_with_metadata(int frontendSocket,
                                                          const char *payload,
                                                          size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    agm_buff *buff = nullptr;
    size_t consumed_size = 0;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    buff = (agm_buff *)payload;
    payload += sizeof(agm_buff);
    buff->addr = (uint8_t *)payload;
    payload += buff->size;
    buff->metadata = (uint8_t *)payload;
    payload += buff->metadata_size;
    memcpy(&consumed_size, payload, sizeof(consumed_size));
    payload += sizeof(consumed_size);

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_write_with_metadata(*handle, buff,
                                                             &consumed_size);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst)
        write_check(frontendSocket, &consumed_size, sizeof(consumed_size));
}

void AgmIpcLsBackend::ipc_agm_session_read_with_metadata(int frontendSocket,
                                                         const char *payload,
                                                         size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    agm_buff *buff = nullptr;
    uint8_t *swap_addr = nullptr;
    uint8_t *swap_metadata = nullptr;
    uint32_t captured_size = 0;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    buff = (agm_buff *)payload;
    payload += sizeof(agm_buff);
    memcpy(&captured_size, payload, sizeof(captured_size));
    payload += sizeof(captured_size);
    std::swap(swap_addr, buff->addr);
    std::swap(swap_metadata, buff->metadata);
    if (buff->size > 0) {
        buff->addr = (uint8_t *)malloc(buff->size);
        if (!buff->addr) {
            AGM_LOGE("malloc failed");
            rst = -1;
            goto end;
        }
    }
    if (buff->metadata_size > 0) {
        buff->metadata = (uint8_t *)malloc(buff->metadata_size);
        if (!buff->metadata) {
            AGM_LOGE("malloc failed");
            rst = -1;
            goto end;
        }
    }

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_read_with_metadata(*handle, buff,
                                                            &captured_size);
end:
    write_check(frontendSocket, &rst, sizeof(rst));
    std::swap(swap_addr, buff->addr);
    std::swap(swap_metadata, buff->metadata);
    if (!rst) {
        write_check(frontendSocket, buff, sizeof(struct agm_buff));
        if (buff->size && swap_addr)
            write_check(frontendSocket, swap_addr, buff->size);
        if (buff->metadata_size && swap_metadata)
            write_check(frontendSocket, swap_metadata, buff->metadata_size);
        write_check(frontendSocket, &captured_size, sizeof(captured_size));
    }

    if (swap_addr)
        free(swap_addr);
    if (swap_metadata)
        free(swap_metadata);
}

void AgmIpcLsBackend::ipc_agm_session_set_non_tunnel_mode_config(
    int frontendSocket, const char *payload, size_t payloadSize) {
    int rst = 0;
    uint64_t *handle = nullptr;
    agm_session_config *session_config = nullptr;
    agm_media_config *in_media_config = nullptr;
    agm_media_config *out_media_config = nullptr;
    agm_buffer_config *in_buffer_config = nullptr;
    agm_buffer_config *out_buffer_config = nullptr;
    const char *pEnd = payload + payloadSize;

    handle = (uint64_t *)payload;
    payload += sizeof(uint64_t);
    session_config = (agm_session_config *)payload;
    payload += sizeof(agm_session_config);
    in_media_config = (agm_media_config *)payload;
    payload += sizeof(agm_media_config);
    out_media_config = (agm_media_config *)payload;
    payload += sizeof(agm_media_config);
    in_buffer_config = (agm_buffer_config *)payload;
    payload += sizeof(agm_buffer_config);
    out_buffer_config = (agm_buffer_config *)payload;
    payload += sizeof(agm_buffer_config);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_session_set_non_tunnel_mode_config(
        *handle, session_config, in_media_config, out_media_config,
        in_buffer_config, out_buffer_config);

    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_get_group_aif_info_list(int frontendSocket,
                                                      const char *payload,
                                                      size_t payloadSize) {
    int rst = 0;
    aif_info *aif_list = nullptr;
    size_t num_groups = 0;
    const char *pEnd = payload + payloadSize;

    memcpy(&num_groups, payload, sizeof(num_groups));
    payload += sizeof(num_groups);

    if (num_groups > 0) {
        aif_list = (aif_info *)malloc(num_groups * sizeof(aif_info));
        if (!aif_list) {
            AGM_LOGE("malloc failed");
            rst = -1;
            write_check(frontendSocket, &rst, sizeof(rst));
            goto end;
        }
    }

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_get_group_aif_info_list(aif_list, &num_groups);
    write_check(frontendSocket, &rst, sizeof(rst));
    if (!rst) {
        write_check(frontendSocket, &num_groups, sizeof(num_groups));
        if (aif_list) {
            write_check(frontendSocket, aif_list,
                        num_groups * sizeof(aif_info));
        }
    }
end:
    if (aif_list)
        free(aif_list);
}

void AgmIpcLsBackend::ipc_agm_aif_group_set_media_config(int frontendSocket,
                                                         const char *payload,
                                                         size_t payloadSize) {
    int rst = 0;
    uint32_t *aif_group_id = nullptr;
    agm_group_media_config *media_config = nullptr;
    const char *pEnd = payload + payloadSize;

    aif_group_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    media_config = (agm_group_media_config *)payload;
    payload += sizeof(agm_group_media_config);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_aif_group_set_media_config(*aif_group_id,
                                                            media_config);

    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_session_write_datapath_params(
    int frontendSocket, const char *payload, size_t payloadSize) {
    int rst = 0;
    uint32_t *session_id = nullptr;
    agm_buff *buff = nullptr;
    const char *pEnd = payload + payloadSize;

    session_id = (uint32_t *)payload;
    payload += sizeof(uint32_t);
    buff = (agm_buff *)payload;
    payload += sizeof(agm_buff);
    buff->addr = (uint8_t *)payload;
    payload += buff->size;
    buff->metadata = (uint8_t *)payload;
    payload += buff->metadata_size;

    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst =
        agmLsBeWrapper.ipc_agm_session_write_datapath_params(*session_id, buff);
    write_check(frontendSocket, &rst, sizeof(rst));
}

void AgmIpcLsBackend::ipc_agm_dump(int frontendSocket, const char *payload,
                                   size_t payloadSize) {
    int rst = 0;
    agm_dump_info *dump_info = nullptr;
    const char *pEnd = payload + payloadSize;

    dump_info = (agm_dump_info *)payload;
    payload += sizeof(agm_dump_info);
    CHECK_PAYLOAD_ADDR(payload, pEnd);
    rst = agmLsBeWrapper.ipc_agm_dump(dump_info);

    write_check(frontendSocket, &rst, sizeof(rst));
}
