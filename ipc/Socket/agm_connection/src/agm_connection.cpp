/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "agm_connection"
#include "inc/agm_connection.h"
#include <log/log.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>

using namespace std;

#define ROUND_UP(x,align) (((x) + (align - 1)) & ~(align - 1))
#define ALIGN16(x) ROUND_UP(x,16)

/* Manage all packets in AgmSocketClient */
struct AgmPacketHdr {
    uint64_t msg_id; /* unique id from certain client */
    uint16_t msg_type; /* request , reply or callback */
    uint16_t cmd; /* function call id */
    uint32_t payload_size; /* size of the packet's payload */
};

struct AgmPacket {
    struct AgmPacketHdr hdr; /* packet head */
    uint8_t payload[0]; /* payload for command */
};

struct AgmPacketCom {
    uint32_t payload_capacity; /* max payload size */
    struct AgmPacket pkt; /* packet */
};

uint64_t getUniqueId() {
    static mutex lock;
    static uint64_t unique = 1;
    lock_guard<mutex> lobj(lock);
    return unique++;
}

AgmSocket::AgmSocket(int fd)
    :mSocketfd(fd),
    mState(SOCKET_STATE_WORKING)
{
    prepareBuffForPayload(16);
    prepareBuffForPayload(128);
    prepareBuffForPayload(4096);
    prepareBuffForPayload(16384);
}

AgmSocket::~AgmSocket()
{
    ALOGE("socket %d quit", mSocketfd);
    shutdown(mSocketfd, SHUT_RDWR);
    close(mSocketfd);
    if(mThread.joinable()) {
        mThread.join();
    }
    for(void* p: mPackets) {
        struct AgmPacketCom* ptr = reinterpret_cast<struct AgmPacketCom*>(p);
        free(ptr);
    }
}

void* AgmSocket::prepareBuffForPayload(uint32_t size) const
{
    struct AgmPacketCom* ret = nullptr;
    for (auto itr = mPackets.begin(); itr != mPackets.end(); itr++) {
        ret = reinterpret_cast<struct AgmPacketCom*>(*itr);
        if (ret && (ret->payload_capacity >= size))
            return ret;
    }

    auto required = sizeof(struct AgmPacketCom) + ALIGN16(size);
    ret = reinterpret_cast<struct AgmPacketCom*>(malloc(required));
    if (ret) {
        ret->payload_capacity = ALIGN16(size);
        mPackets.push_back(ret);
        ALOGW("prepared packet for payload size %zu payload_capacity %zu", size, ret->payload_capacity);
    } else {
        ALOGE("packet allocate failed!");
    }
    return ret;
}

int AgmSocket::Send(uint16_t cmd,
                uint16_t msg_type,
                uint32_t size,
                agmConnFillPayload fillPacket) const
{
    struct AgmPacketCom* p = nullptr;
    struct msghdr msg;
    struct iovec iov;
    int ret = 0;

    memset(&msg, 0, sizeof(msg));
    memset(&iov, 0, sizeof(iov));
    ALOGV("%s size %u", __func__, size);

    p = reinterpret_cast<struct AgmPacketCom*>(prepareBuffForPayload(size));
    if (nullptr == p) {
        ALOGE("AgmPacketCom malloc failed %s", strerror(errno));
        return -ENOMEM;
    }

    p->pkt.hdr.msg_id = getUniqueId();
    p->pkt.hdr.cmd = cmd;
    p->pkt.hdr.payload_size = size;
    p->pkt.hdr.msg_type = msg_type;
    if (size) {
        fillPacket(p->pkt.payload);
    }

    iov.iov_base = &p->pkt;
    /* just send the necessary data */
    iov.iov_len = sizeof(struct AgmPacketHdr) + size;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;

    ALOGV("%s cmd %u size %u", __func__, cmd, iov.iov_len);
    ret = sendmsg(mSocketfd, &msg, MSG_NOSIGNAL);
    return ret;
}

int AgmSocket::Receive(const agmCmdHandler& handler) const
{
    struct AgmPacketCom* p = nullptr;
    struct msghdr msg;
    struct iovec iov;
    int ret = 0;
    struct AgmPacketHdr hdr = {0};

    memset(&msg, 0, sizeof(msg));
    memset(&iov, 0, sizeof(iov));
    memset(&hdr, 0, sizeof(hdr));

    iov.iov_base = &hdr;
    iov.iov_len = sizeof(struct AgmPacketHdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    ALOGV("debug waiting msg");
    ret = recvmsg(this->mSocketfd, &msg, 0);

    if (ret <= 0) {
        ALOGW("recv end with error %s", strerror(errno));
        mState = SOCKET_STATE_STOP;
        return -1;
    } else {
        ALOGV("recv size %d", ret);
    }

    uint16_t cmd = hdr.cmd;
    uint16_t msg_type = hdr.msg_type;
    uint64_t size = hdr.payload_size;
    ALOGV("read hdr, got payload msg_id %lld cmd %u type %u size %d", hdr.msg_id, cmd, msg_type, size);
    if (size == 0) {
        handler(cmd, msg_type, 0, nullptr, *this);
        return size;
    }

    p = reinterpret_cast<struct AgmPacketCom*>(prepareBuffForPayload(size));
    if (nullptr == p) {
        ALOGE("AgmPacketCom malloc failed %s", strerror(errno));
        return -ENOMEM;
    }

    memset(&msg, 0, sizeof(msg));
    memset(&iov, 0, sizeof(iov));
    iov.iov_base = p->pkt.payload;
    iov.iov_len = size;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    msg.msg_controllen = 0;
    ret = recvmsg(mSocketfd, &msg, 0);
    ALOGV("read payload size %d", ret);
    handler(cmd, msg_type, size, p->pkt.payload, *this);
    return size;
}

void AgmSocket::handleInLoop(function<int(const AgmSocket&)> workfunc)
{
    thread t([this, workfunc]() {
        do {
            workfunc(*this);
        } while (this->mState == SOCKET_STATE_WORKING);
    });
    mThread.swap(t);
}

AgmSocketClient* AgmSocketClient::mInstance = nullptr;

AgmSocketClient::AgmSocketClient(int fd)
    :AgmSocket(fd)
{
    ALOGV("%s", __func__);
}

AgmSocketClient::~AgmSocketClient()
{
    ALOGW("%s quit", __func__);
}

AgmSocketClient* AgmSocketClient::getInstance()
{
    ALOGV("%s", __func__);

    static once_flag once;
    call_once(once, []() {
        int fd = 0;
        int ret = 0;
        struct sockaddr_un addr = {0};
        memset(&addr, 0, sizeof(addr));

        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        while (fd < 0) {
            ALOGE("none socket avaible!");
            fd = socket(AF_UNIX, SOCK_STREAM, 0);
        }
        ALOGV("%s fd %d", __func__, fd);

        addr.sun_family = AF_UNIX;
        snprintf(addr.sun_path, sizeof(addr.sun_path), "%s_%d", AGM_SOCKET_PATH, getpid());

        ret = ::bind(fd, (struct sockaddr*)&addr, sizeof(addr));
        while (ret < 0) {
            ALOGE("socket bind failed!");
            usleep(1000);
            ret = ::bind(fd, (struct sockaddr*)&addr, sizeof(addr));
        }
        /* system & audioserver read write */
        chmod(addr.sun_path, 0777);
        mInstance = new AgmSocketClient(fd);
        mInstance->Connect();
    });
    return mInstance;
}

int AgmSocketClient::Connect()
{
    int ret = 0;
    int count = 0;
    struct sockaddr_un addr = {0};
    memset(&addr, 0, sizeof(addr));

    ALOGV("%s", __func__);
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s_0", AGM_SOCKET_PATH);

    while (ret = connect(mSocketfd, (struct sockaddr*)&addr, sizeof(addr))) {
        ALOGW("Connect error %s", strerror(ret));
        if (ENOENT == ret ||  ECONNREFUSED  == ret || EACCES == ret) {
            count++;
            if (count < 10)
                usleep(1000);
            else
                break;
        } else {
            break;
        }
    }
    return ret;
}

AgmSocketServer* AgmSocketServer::mInstance = nullptr;

AgmSocketServer::AgmSocketServer(int fd ,function<int(const AgmSocket&)> func)
    :mSocketfd(fd),
    mWorkFunc(func),
    mReady(true)
{
    ALOGV("%s", __func__);
}

AgmSocketServer::~AgmSocketServer()
{
    ALOGV("stop accept");
    mReady = false;
    shutdown(mSocketfd, SHUT_RDWR);
    close(mSocketfd);
    if (mAcceptThread.joinable()) {
        mAcceptThread.join();
    }
    ALOGV("stop worker");
    for (auto& work: mSubWorkingSockets) {
        if (work) {
            delete work;
        }
        work = nullptr;
    }
}

AgmSocketServer* AgmSocketServer::getInstance(function<int(const AgmSocket&)> func)
{
    static once_flag once;

    ALOGV("%s", __func__);
    call_once(once, [&func]() {
        struct sockaddr_un addr = {0};
        int fd = 0;
        int ret = 0;
        memset(&addr, 0, sizeof(addr));

        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        while (fd < 0) {
            ALOGE("none socket avaible!");
            fd = socket(AF_UNIX, SOCK_STREAM, 0);
        }
        ALOGV("%s fd %d", __func__, fd);

        addr.sun_family = AF_UNIX;
        snprintf(addr.sun_path, sizeof(addr.sun_path), "%s_0", AGM_SOCKET_PATH);
        ::unlink(addr.sun_path);

        ret = ::bind(fd, (struct sockaddr*)&addr, sizeof(addr));
        while (ret < 0) {
            ALOGE("socket bind failed! %d perror %d strerr %s",ret,errno,strerror(errno));
            perror("bind failed");
            usleep(1000);
            ret = ::bind(fd, (struct sockaddr*)&addr, sizeof(addr));
        }
        /* system & audioserver read write */
        chmod(addr.sun_path, 0777);
        /* make socket a server */
        ret = ::listen(fd, 4);
        while (ret < 0) {
            ALOGE("socket listen failed!");
            ret = ::listen(fd, 4);
        }
        mInstance = new AgmSocketServer(fd, func);
        mInstance->Accept();
    });
    return mInstance;
}

void AgmSocketServer::releaseInstance()
{
    if (mInstance) {
        delete mInstance;
        mInstance = nullptr;
    }
}

void AgmSocketServer::createSubSocket(int fd)
{

    ALOGV("%s fd %d", __func__, fd);
    AgmSocket* as = new AgmSocket(fd);
    as->handleInLoop(mWorkFunc);

    mSubWorkingSockets.push_back(as);
}

void AgmSocketServer::Accept()
{
    ALOGV("%s", __func__);
    thread t([this]()
        {
            int fd = 0;
            struct sockaddr_un addr = {0};
            memset(&addr, 0, sizeof(addr));
            socklen_t len = sizeof(addr);
            do {
                fd = accept(this->mSocketfd, (struct sockaddr*)&addr, &len);
                if (fd > 0) {
                    this->createSubSocket(fd);
                }
            } while(mReady);
        }
    );
    mAcceptThread.swap(t);
}
