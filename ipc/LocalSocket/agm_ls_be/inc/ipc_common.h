/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.  */
/* SPDX-License-Identifier: BSD-3-Clause-Clear */

#ifndef IPC_COMMON_H
#define IPC_COMMON_H

#define AGM_SOCKET_PATH "/dev/socket/agm/ipc"
#define AGM_CB_SOCKET_PATH "/dev/socket/agm/ipc_cb"

// Define opcodes for different agm functions
enum class Opcode {
    AGM_AIF_SET_MEDIA_CONFIG = 1,
    AGM_AIF_SET_METADATA,
    AGM_SESSION_SET_METADATA,
    AGM_SESSION_AIF_SET_METADATA,
    AGM_SESSION_AIF_CONNECT,
    AGM_SESSION_AIF_GET_TAG_MODULE_INFO,
    AGM_AIF_SET_PARAMS,
    AGM_SESSION_AIF_SET_PARAMS,
    AGM_SESSION_AIF_SET_CAL,
    AGM_SESSION_SET_PARAMS,
    AGM_SESSION_GET_PARAMS,
    AGM_GET_PARAMS_FROM_ACDB_TUNNEL,
    AGM_SET_PARAMS_WITH_TAG,
    AGM_SET_PARAMS_WITH_TAG_TO_ACDB,
    AGM_SET_PARAMS_TO_ACDB_TUNNEL,
    AGM_SESSION_REGISTER_CB,
    AGM_SESSION_REGISTER_FOR_EVENTS,
    AGM_SESSION_OPEN,
    AGM_SESSION_SET_CONFIG,
    AGM_SESSION_CLOSE,
    AGM_SESSION_PREPARE,
    AGM_SESSION_START,
    AGM_SESSION_STOP,
    AGM_SESSION_PAUSE,
    AGM_SESSION_FLUSH,
    AGM_SESSIONID_FLUSH,
    AGM_SESSION_RESUME,
    AGM_SESSION_SUSPEND,
    AGM_SESSION_READ,
    AGM_SESSION_WRITE,
    AGM_GET_HW_PROCESSED_BUFF_CNT,
    AGM_GET_AIF_INFO_LIST,
    AGM_SESSION_SET_LOOPBACK,
    AGM_SESSION_SET_EC_REF,
    AGM_SESSION_EOS,
    AGM_GET_SESSION_TIME,
    AGM_GET_BUFFER_TIMESTAMP,
    AGM_SESSION_GET_BUF_INFO,
    AGM_REGISTER_SERVICE_CRASH_CALLBACK,
    AGM_SET_GAPLESS_SESSION_METADATA,
    AGM_SESSION_WRITE_WITH_METADATA,
    AGM_SESSION_READ_WITH_METADATA,
    AGM_SESSION_SET_NON_TUNNEL_MODE_CONFIG,
    AGM_GET_GROUP_AIF_INFO_LIST,
    AGM_AIF_GROUP_SET_MEDIA_CONFIG,
    AGM_SESSION_WRITE_DATAPATH_PARAMS,
    AGM_DUMP,
    DISCONNECT, // For handling client disconnection
};

enum class Cbcode {
    AGM_EVETN_CB = 1,
    AGM_SERVICE_CRASH_CB,
};

// IPC Header struct
struct IPCHeader {
    size_t totalLength; // Total length of the message (header + payload)
    Opcode opcode;      // Operation code
};

// IPC Header struct
struct IPCCbHeader {
    size_t totalLength; // Total length of the message (header + payload)
    Cbcode cbcode;      // Operation code
};

#endif // IPC_COMMON_H
