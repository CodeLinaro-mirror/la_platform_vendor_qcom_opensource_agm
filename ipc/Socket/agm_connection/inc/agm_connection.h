/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _AGM_CONN_H_
#define _AGM_CONN_H_

#include <mutex>
#include <vector>
#include <thread>
#include <sys/socket.h>
#include <agm/agm_api.h>

#define AGM_SOCKET_PATH "/dev/socket/agm/agm_socket"
#define AGM_SOCKET_DIR "/dev/socket/agm/"

enum AGM_CMD {
    AGM_CMD_GET_AIF_INFO_LIST,
    AGM_CMD_GET_GROUP_AIF_INFO_LIST,
    AGM_CMD_SESSION_SET_METADATA,
    AGM_CMD_SESSION_AIF_GET_TAG_MODULE_INFO,
    AGM_CMD_SESSION_AIF_SET_PARAMS,
    AGM_CMD_HW_SRC_CONFIG,
    AGM_CMD_AIF_SET_MEDIA_CONFIG,
    AGM_CMD_AIF_SET_METADATA,
    AGM_CMD_SESSION_AIF_SET_METADATA,
    AGM_CMD_SESSION_AIF_CONNECT,
    AGM_CMD_SESSION_OPEN,
    AGM_CMD_SESSION_SET_CONFIG,
    AGM_CMD_SESSION_PREPARE,
    AGM_CMD_SESSION_START,
    AGM_CMD_SESSION_STOP,
    AGM_CMD_SESSION_CLOSE,
    AGM_CMD_SET_PARAMS_WITH_TAG_TO_ACDB,
    AGM_CMD_SESSION_WRITE,
    AGM_CMD_SESSION_HEARTBEAT,
    AGM_CMD_MAX
};

enum AGM_CMD_TYPE {
    AGM_CMD_TYPE_REQUEST,
    AGM_CMD_TYPE_REPLY,
    AGM_CMD_TYPE_CALLBACK,
    AGM_CMD_TYPE_MAX
};

enum AgmServerStatus {
    AGM_SERVER_DIED = 0,
    AGM_SERVER_ALIVE
};

/* clients and server can run on 32 or 64
 * need to align the data struct
 */
struct AifInfo_Socket {
    char aif_name[AIF_NAME_MAX_LEN];          /**< AIF name  */
    uint32_t dir;               /**< direction */
};

struct AgmMediaConfig_Socket {
    uint32_t rate;                 /**< sample rate */
    uint32_t channels;             /**< number of channels */
    uint32_t format;  /**< format */
    uint32_t data_format;          /**< data format */
};

struct AgmSessionConfig_Socket {
    uint32_t dir;        /**< TX or RX */
    uint32_t sess_mode;  /**< indicates mode of agm sesison, non-tunnel, or hostless */
    uint32_t start_threshold;  /**< start_threshold: number of buffers * buffer size */
    uint32_t stop_threshold;   /**< stop_th6reshold: number of buffers * buffer size */
    union agm_session_codec codec; /**< codec configuration - union with fixed format */
    uint32_t data_mode; /**< compress format ID */
    uint32_t sess_flags; /**< pass session specific flags e.g enable inband SRCM event*/
};

struct AgmBufferConfig_Socket {
    uint32_t count; /**< number of buffers */
    uint32_t size;    /**< size of each buffer */
    uint32_t max_metadata_size; /**< max metadata size a client attaches to a buffer */
};

class AgmSocket;
using agmConnFillPayload = std::function<void(uint8_t*)>;
/* void (cmd, msg_type, size, payload, AgmSocket) */
using agmCmdHandler = std::function<void(uint16_t,uint16_t,uint32_t,uint8_t*, const AgmSocket&)>;

enum SOCKET_STATE {
    SOCKET_STATE_WORKING,
    SOCKET_STATE_STOP,
    SOCKET_STATE_MAX
};

class AgmSocket {
public:
    AgmSocket(int32_t fd=0);

    ~AgmSocket();
    /*
    * \brief
    * \param[in] cmd - AGM_CMD
    * \param[in] msg_type - AGM_CMD_TYPE
    * \param[in] payload_size - size of the payload
    * \param[in] fillPayload - function to fill the payload
    *
    * \return the size of data by the transaction
    */
    int Send(uint16_t cmd, uint16_t msg_type, uint32_t payload_size, agmConnFillPayload fillPayload) const;

    /*
    * \brief
    * \param[out] cmd - AGM_CMD
    * \param[out] size - Payload size
    * \param[out] msg_type - AGM_CMD_TYPE
    * \param[in] extractPayload - function to extract the payload
    *
    * \return the size of data by the transaction
    */
    int Receive(const agmCmdHandler& handler) const;

    /*
    * \brief
    * \param[in] workfunc - function to execute command
    *
    * \return the size of data by the transaction
    */
    void handleInLoop(std::function<int(const AgmSocket&)> workfunc);

    int32_t mSocketfd;

    mutable std::vector<void*> mPackets;

    mutable enum SOCKET_STATE mState;
private:
    void* prepareBuffForPayload(uint32_t size) const;
    std::thread mThread;
    std::function<int(const AgmSocket&)> mWorkFunc;
};

class AgmSocketClient: public AgmSocket {
public:
    static AgmSocketClient* getInstance();
    ~AgmSocketClient();
private:
    AgmSocketClient(int fd);
    static AgmSocketClient* mInstance;
    int Connect();
};

class AgmSocketServer {
public:
    ~AgmSocketServer();
    static AgmSocketServer* getInstance(std::function<int(const AgmSocket&)> func);
    /* socket server never up again if released */
    static void releaseInstance();
    void createSubSocket(int32_t fd);
private:
    AgmSocketServer(int fd, std::function<int(const AgmSocket&)> func);
    static AgmSocketServer* mInstance;
    void Accept();

    int32_t mSocketfd;
    std::vector<AgmSocket*> mSubWorkingSockets;
    std::function<int(const AgmSocket&)> mWorkFunc;
    std::thread mAcceptThread;
    bool mReady;
};

#endif
