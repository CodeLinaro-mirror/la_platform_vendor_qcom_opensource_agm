/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <agm/utils.h>
#include <agm/codec/codec_interface.h>
#include <agm/codec/cdc_interface.h>
#include <agm/codec/codec_core_manager.h>
#include <agm/codec/CodecModuleList.h>
#include <errno.h>
#ifdef CDC_USE_CUTILS
#include <sys/poll.h>
#else
#include "poll.h"
#endif
#include<unistd.h>

#define CDC_DRIVER_PATH "/dev/glinkpkt_q6_audio_ctrl"
#define CDC_MAX_RECEIVE_BUF_SIZE (4096)
#define CDC_NUM_FDS 2
#define CDC_TIMEOUT_NS(x)  ((x) * (1000LL) * (1000LL))
#define CDC_TIMEOUT_US(x)  ((x) * (1000LL))
#define CDC_TXN_TIMEOUT_MS  (1000) // 2 sec
#define CDC_PADDING_8BYTE_ALIGN(x)  ((((x) + 7) & 7) ^ 7)

struct ep_obj_pool {
    struct listnode ep_obj_list;
    pthread_mutex_t lock;
};

struct ep_obj {
    struct listnode node;
    uint32_t ep_id;
    cdc_command_t *cmd;
    cdc_response_t *rsp;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

typedef struct cdc_dl {
    pthread_t receiver_thread;
    bool thread_exit;
    int drv_fd;
    int intpipe[2];
    uint8_t recv_buf[CDC_MAX_RECEIVE_BUF_SIZE];
} cdc_dl_t;

int cdc_ipc_init();
int cdc_ipc_deinit();
int cdc_ipc_send(struct ep_obj *obj, cdc_command_t *cmd, uint32_t cmd_size);
int32_t ep_obj_response_timedwait(struct ep_obj *ep_obj, int64_t timeout_in_nsec);
void *cdc_ipc_receive(void *);

static pthread_mutex_t cdc_interface_lock = PTHREAD_MUTEX_INITIALIZER;
static int init_ref;
cdc_dl_t cdc_dl;
static struct ep_obj_pool *ep_pool;

static int ep_pool_init()
{
    int ret = 0; 
    ep_pool = calloc(1, sizeof(struct ep_obj_pool));
    if (!ep_pool) {
        AGM_LOGE("No Memory to create ep_pool\n");
        ret = -ENOMEM;
        goto done;
    }    
    list_init(&ep_pool->ep_obj_list);
    pthread_mutex_init(&ep_pool->lock, (const pthread_mutexattr_t *) NULL);

done:
    return ret; 
}

static struct ep_obj* ep_obj_create(int ep_id)
{
    struct ep_obj *obj = NULL;
    pthread_condattr_t attr;
    int ret = 0;

    obj = calloc(1, sizeof(struct ep_obj));
    if (!obj) {
        AGM_LOGE("Memory allocation failed for end point object\n");
        return obj;
    }

    obj->ep_id = ep_id;
    pthread_mutex_init(&obj->lock, (const pthread_mutexattr_t *) NULL);
    if (ret) {
        AGM_LOGE("%s: Failed to init mutex, rc = %d\n", __func__, ret);
    }
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    ret = pthread_cond_init(&obj->cond, &attr);
    if (ret) {
        AGM_LOGE("%s: Failed to init cond, rc = %d\n", __func__, ret);
    }

    return obj;
}

struct ep_obj *ep_obj_retrieve_from_pool(uint32_t ep_id)
{
    struct ep_obj *obj = NULL;
    struct listnode *node;

    pthread_mutex_lock(&ep_pool->lock);
    list_for_each(node, &ep_pool->ep_obj_list) {
        obj = node_to_item(node, struct ep_obj, node);
        if (obj->ep_id == ep_id)
            break;
        else
            obj = NULL;
    }
    pthread_mutex_unlock(&ep_pool->lock);

    return obj;
}

struct ep_obj *ep_obj_get_from_pool(uint32_t ep_id)
{
    struct ep_obj *obj = NULL;
    struct listnode *node;

    pthread_mutex_lock(&ep_pool->lock);
    list_for_each(node, &ep_pool->ep_obj_list) {
        obj = node_to_item(node, struct ep_obj, node);
        if (obj->ep_id == ep_id)
            break;
        else
            obj = NULL;
    }

    if (!obj) {
        AGM_LOGE("Couldnt find a end point object in the list, creating one\n");
        obj = ep_obj_create(ep_id);
        if (!obj) {
            AGM_LOGE("Couldnt create a end point object\n");
            goto done;
        }
        list_add_tail(&ep_pool->ep_obj_list, &obj->node);
    }

done:
    pthread_mutex_unlock(&ep_pool->lock);
    return obj;
}

static void ep_obj_free(struct ep_obj *ep_obj)
{
    if (ep_obj->cmd)
        free(ep_obj->cmd);
    if (ep_obj->rsp)
        free(ep_obj->rsp);
    free(ep_obj);
}

static void ep_pool_free()
{
    struct ep_obj *ep_obj;
    struct listnode *node, *next;
    int ret = 0;

    pthread_mutex_lock(&ep_pool->lock);
    list_for_each_safe(node, next, &ep_pool->ep_obj_list) {
        ep_obj = node_to_item(node, struct ep_obj, node);
        list_remove(&ep_obj->node);
        ep_obj_free(ep_obj);
    }
    pthread_mutex_unlock(&ep_pool->lock);
    free(ep_pool);
}


/**
\brief initializes codec interface
*/
int cdc_interface_init()
{
    int32_t ret = 0;

    pthread_mutex_lock(&cdc_interface_lock);
    if (++init_ref == 1)
    {
        ret = ep_pool_init();
        if (ret) {
            goto exit;
        }

        ret = cdc_ipc_init();
    }
exit:
    if(ret)
        --init_ref;
    AGM_LOGI("%s - init ref %d\n", __func__, init_ref);
    pthread_mutex_unlock(&cdc_interface_lock);
    return ret;
}

int cdc_interface_deinit()
{
    int32_t ret = 0;

    pthread_mutex_lock(&cdc_interface_lock);
    if (init_ref && (1 == init_ref--)) 
    {
        // cdc_ipc_deint should stop and join the reciever thread
        // only then free the ep_pool, since reciever thread access the pool
        ret = cdc_ipc_deinit();
        if (!ret)
           ep_pool_free();
    }
    pthread_mutex_unlock(&cdc_interface_lock);
    AGM_LOGI("%s - init ref %d\n", __func__, init_ref);
    return ret;
}

int cdc_route_endpoint(int ep_id, struct codec_module_payload_list *mp_list, bool set)
{
    struct ep_obj *ep_obj = NULL;
    cdc_command_t *cmd;
    size_t cmd_size = 0;
    ep_cmd_route_payload_t *ep_cmd;
    uint32_t ep_cmd_size = 0;
    struct cdc_module_payload *module_header = NULL;
    struct codec_module_payload *module_payload = NULL;
    uint32_t total_module_payload_size = 0;
    int32_t current_cmd_opcode;
    int32_t pad_bytes = 0;
    int32_t ret = 0;

    current_cmd_opcode = set ? CODEC_CMD_ENDPOINT_SET_ROUTING : CODEC_CMD_ENDPOINT_RESET_ROUTING;
    ep_obj = ep_obj_get_from_pool(ep_id);
    if (!ep_obj) {
        ret = -ENOMEM;
        goto exit;
    }

    pthread_mutex_lock(&ep_obj->lock);
    if (ep_obj->cmd) {
        ret = -EBUSY;
        AGM_LOGE("%s: pending command 0x%X with ep_id %d\n",
               __func__, ep_obj->cmd->cmd_opcode, ep_obj->ep_id);
        goto unlock;
    }

    total_module_payload_size = mp_list->num_mp * 
                                sizeof(struct cdc_module_payload) +
                                mp_list->total_payload_size;
    cmd_size = sizeof(cdc_command_t) +
                sizeof(ep_cmd_route_payload_t) +
                total_module_payload_size;
    pad_bytes = CDC_PADDING_8BYTE_ALIGN(cmd_size);

    cmd = (cdc_command_t*)calloc(cmd_size + pad_bytes, 1);
    if (!cmd) {
        ret = -ENOMEM;
        goto unlock;
    }
    cmd->cmd_opcode = current_cmd_opcode;
    AGM_LOGI("%s: process command 0x%X with ep_id %d\n",
            __func__, cmd->cmd_opcode, ep_obj->ep_id);

    ep_cmd = (ep_cmd_route_payload_t*)(cmd->cmd_payload);
    ep_cmd->epid = ep_id;
    ep_cmd->ep_payload_size = total_module_payload_size;

    module_header = (struct cdc_module_payload*)(ep_cmd->ep_payload);
    for (int i = 0; i < mp_list->num_mp; i++)
    {
        module_payload = mp_list->mp_arr + i;
        module_header->module_id = module_payload->mid;
        module_header->size = module_payload->size;
        memcpy(module_header->byte, module_payload->data, module_payload->size);
        module_header = (struct cdc_module_payload*)(module_header->byte + module_header->size);
    }

    ret = cdc_ipc_send(ep_obj, cmd, cmd_size);
    if (ret){
        AGM_LOGI("%s: process command 0x%X with ep_id %d failed : ret %d\n",
                __func__, current_cmd_opcode, ep_obj->ep_id, ret);
    } else {
        if (ep_obj->rsp) {
            free(ep_obj->rsp);
            ep_obj->rsp = NULL; 
        }
    }

    if (ep_obj->cmd) {
        free(ep_obj->cmd);
        ep_obj->cmd = NULL;
    }

unlock:
    AGM_LOGI("%s: processed command 0x%X with ep_id %d : ret %d\n",
            __func__, current_cmd_opcode, ep_obj->ep_id, ret);
    pthread_mutex_unlock(&ep_obj->lock);
exit:
    return ret;
}

int cdc_enable_endpoint(int ep_id, struct codec_media_config *config)
{
    struct ep_obj *ep_obj = NULL;
    cdc_command_t *cmd;
    uint32_t cmd_size = 0;
    ep_cmd_enable_payload_t *ep_cmd_enable = NULL;
    int32_t current_cmd_opcode = CODEC_CMD_ENDPOINT_ENABLE;
    int32_t pad_bytes = 0;
    int32_t ret = 0;

    ep_obj = ep_obj_get_from_pool(ep_id);
    if (!ep_obj) {
        ret = -ENOMEM;
        goto exit;
    }

    pthread_mutex_lock(&ep_obj->lock);
    if (ep_obj->cmd) {
        ret = -EBUSY;
        AGM_LOGE("%s: pending command 0x%X with ep_id %d\n",
               __func__, ep_obj->cmd->cmd_opcode, ep_obj->ep_id);
        goto unlock;
    }

    cmd_size = sizeof(cdc_command_t) + sizeof(ep_cmd_enable_payload_t);
    pad_bytes = CDC_PADDING_8BYTE_ALIGN(cmd_size);

    cmd = (cdc_command_t*)calloc(cmd_size + pad_bytes, 1);
    if (!cmd) {
        ret = -ENOMEM;
        goto unlock;
    }
    cmd->cmd_opcode = current_cmd_opcode;
    AGM_LOGI("%s: process command 0x%X with ep_id %d\n",
            __func__, cmd->cmd_opcode, ep_obj->ep_id);

    ep_cmd_enable = (ep_cmd_enable_payload_t*)(cmd->cmd_payload);
    ep_cmd_enable->epid = ep_id;
    ep_cmd_enable->cdc_media_config.sample_rate = config->rate;
    ep_cmd_enable->cdc_media_config.channels = config->ch;
    ep_cmd_enable->cdc_media_config.bit_width = config->bit_width;

    ret = cdc_ipc_send(ep_obj, cmd, cmd_size);
    if (ret){
        AGM_LOGI("%s: process command 0x%X with ep_id %d failed : ret %d\n",
                __func__, current_cmd_opcode, ep_obj->ep_id, ret);
    } else {
        if (ep_obj->rsp) {
            free(ep_obj->rsp);
            ep_obj->rsp = NULL; 
        }
    }

    if (ep_obj->cmd) {
        free(ep_obj->cmd);
        ep_obj->cmd = NULL;
    }

unlock:
    AGM_LOGI("%s: processed command 0x%X with ep_id %d : ret %d\n",
            __func__, current_cmd_opcode, ep_obj->ep_id, ret);
    pthread_mutex_unlock(&ep_obj->lock);

exit:
    return ret;
}

int cdc_set_custom_payload_endpoint(int param_id, void* ep_payload, int payload_size)
{
    AGM_LOGI("%s\n", __func__);
    //TBD for a Bluetooth usecase
    return 0;
}

int cdc_disable_endpoint(int ep_id)
{
    struct ep_obj *ep_obj = NULL;
    cdc_command_t *cmd = NULL;
    ep_cmd_disable_payload_t *ep_cmd_disable = NULL;
    int32_t current_cmd_opcode = CODEC_CMD_ENDPOINT_DISABLE;
    uint32_t cmd_size = 0;
    int32_t pad_bytes = 0;
    int32_t ret = 0;

    ep_obj = ep_obj_get_from_pool(ep_id);
    if (!ep_obj) {
        ret = -ENOMEM;
        goto exit;
    }

    pthread_mutex_lock(&ep_obj->lock);
    if (ep_obj->cmd) {
        ret = -EBUSY;
        AGM_LOGE("%s: pending command 0x%X with ep_id %d\n",
               __func__, ep_obj->cmd->cmd_opcode, ep_obj->ep_id);
        goto unlock;
    }
    cmd_size = sizeof(cdc_command_t) + sizeof(ep_cmd_disable_payload_t);
    pad_bytes = CDC_PADDING_8BYTE_ALIGN(cmd_size);

    cmd = (cdc_command_t*)calloc(cmd_size + pad_bytes, 1);
    if (!cmd) {
        ret = -ENOMEM;
        goto unlock;
    }
    cmd->cmd_opcode = current_cmd_opcode;
    AGM_LOGI("%s: process command 0x%X with ep_id %d\n",
            __func__, cmd->cmd_opcode, ep_obj->ep_id);

    ep_cmd_disable = (ep_cmd_disable_payload_t*)(cmd->cmd_payload);
    ep_cmd_disable->epid = ep_id;

    ret = cdc_ipc_send(ep_obj, cmd, cmd_size);
    if (ret){
        AGM_LOGI("%s: process command 0x%X with ep_id %d failed : ret %d\n",
                __func__, current_cmd_opcode, ep_obj->ep_id, ret);
    } else {
        if (ep_obj->rsp) {
            free(ep_obj->rsp);
            ep_obj->rsp = NULL; 
        }
    }

    if (ep_obj->cmd) {
        free(ep_obj->cmd);
        ep_obj->cmd = NULL;
    }

unlock:
    AGM_LOGI("%s: processed command 0x%X with ep_id %d : ret %d\n",
            __func__, current_cmd_opcode, ep_obj->ep_id, ret);
    pthread_mutex_unlock(&ep_obj->lock);

exit:
    return ret;
}

int cdc_ipc_init()
{
    int32_t ret = 0;
    char *drv_name = NULL;
    pthread_attr_t tattr;
    struct sched_param param = { .sched_priority = 3 };

    AGM_LOGI("%s : Enter\n", __func__);
    cdc_dl.thread_exit = false;
    drv_name = CDC_DRIVER_PATH;
    cdc_dl.drv_fd = open(CDC_DRIVER_PATH, O_RDWR);

    if (cdc_dl.drv_fd < 0) {
        AGM_LOGE("%s:%d driver open failed %d for %s : %s",
                __func__, __LINE__, errno, drv_name, strerror(errno));
        return -EBADF;
    }

    if (pipe(cdc_dl.intpipe) < 0) {
        AGM_LOGE("%s:%d pipe open failed %d", 
                __func__, __LINE__, errno);
        close(cdc_dl.drv_fd);
        return -EBADF;
    }

    pthread_attr_init(&tattr);
    pthread_attr_setschedparam(&tattr, &param);
    pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);
    ret = pthread_create(&cdc_dl.receiver_thread, &tattr,
            cdc_ipc_receive, NULL);
    if (ret) {
        AGM_LOGE("%s:%d error:%d pthread_create fail", __func__, __LINE__, ret);
        cdc_dl.receiver_thread = 0;
        close(cdc_dl.drv_fd);
        cdc_dl.drv_fd = -1;
        close(cdc_dl.intpipe[0]);
        cdc_dl.intpipe[0] = -1;
        close(cdc_dl.intpipe[1]);
        cdc_dl.intpipe[1] = -1;
    }
    AGM_LOGI("%s : Exit - %d\n", __func__, ret);
    return ret;
}

int cdc_ipc_deinit()
{
    int32_t ret = 0;
    AGM_LOGI("%s : Enter\n", __func__);
    /*
     * Set thread exit to true and then close the driver instance
     * this should unblock the poll and then we do a pthread_join
     * to ensure that the receiver_thread has exited.
     */
    cdc_dl.thread_exit = true;
    ret = write(cdc_dl.intpipe[1], "Q", 1);
    if(ret < 0) {
        /* proceed regardless with a error print */
        AGM_LOGE("%s:%d write to driver failed %d", __func__, __LINE__, errno);
    }

    ret = pthread_join(cdc_dl.receiver_thread, NULL);
    if (ret < 0){
        AGM_LOGE("%s:%d pthread_join failed %d", __func__, __LINE__, ret);
        goto exit;
    }
    if (cdc_dl.drv_fd > 0)
        close(cdc_dl.drv_fd);
    cdc_dl.drv_fd = -1;
    if (cdc_dl.intpipe[0] > 0)
        close(cdc_dl.intpipe[0]);
    cdc_dl.intpipe[0] = -1;
    if (cdc_dl.intpipe[1] > 0)
        close(cdc_dl.intpipe[1]);
    cdc_dl.intpipe[1] = -1;
exit:
    AGM_LOGI("%s : Exit\n", __func__);
    return ret;
}

int cdc_ipc_send(struct ep_obj *obj, cdc_command_t *cmd, uint32_t cmd_size)
{
    int32_t ret = 0;

    obj->cmd = cmd;

    ret = write(cdc_dl.drv_fd, cmd, cmd_size);
    if (ret == cmd_size) {
        AGM_LOGI("%s : wait for response\n", __func__);
        ret = ep_obj_response_timedwait(obj,
                            CDC_TIMEOUT_NS(CDC_TXN_TIMEOUT_MS));
        if (ret) {
            if (ret == ETIMEDOUT) {
                AGM_LOGE("%s : Wait timeout %d\n", __func__, ret);
            } else {
                AGM_LOGE("%s : Failed to wait %d\n", __func__, ret);
            }
        } else if (!ret && (obj->rsp == NULL)){
            ret = -ENODATA;
        } else {
            ret = obj->rsp->status;
        }
        AGM_LOGI("%s: Wait done opcode [0x%X] with ep_id [%d] : ret [%d]\n",
                __func__, cmd->cmd_opcode, obj->ep_id, ret);
    } else {
        AGM_LOGI("%s : write failed %s\n", __func__, strerror(errno));
    }

    return ret;
}

void *cdc_ipc_receive(void *priv_data)
{
    uint32_t status;
    int32_t receive_size;
    void *buf;
    uint32_t *temp;
    struct pollfd *pfd;
    cdc_response_t *cmd_rsp = NULL;
    ep_cmd_response_t *ep_rsp = NULL;
    uint32_t rsp_opcode;
    int32_t ep_id;
    int32_t cmd_rsp_size;
    struct ep_obj *ep_obj;

    pfd = (struct pollfd *)calloc(CDC_NUM_FDS, sizeof(struct pollfd));
    if (pfd == NULL) {
        AGM_LOGE("%s:%d calloc failed for poll fd", __func__, __LINE__);
        return NULL;
    }
    if (cdc_dl.drv_fd) {
        pfd[0].fd = cdc_dl.drv_fd;
        pfd[0].events = POLLIN|POLLPRI|POLLERR|POLLHUP|POLLNVAL;
        pfd[1].fd = cdc_dl.intpipe[0];
        pfd[1].events = POLLIN|POLLPRI|POLLERR|POLLHUP|POLLNVAL;
    } else {
        AGM_LOGE("%s:%d invalid driver inst exit resp thread", __func__, __LINE__);
        return NULL;
    }

    AGM_LOGI("%s : start poll", __func__);
    while (1) {
        if (cdc_dl.thread_exit) {
            AGM_LOGE("%s:%d exiting receiver thread", __func__, __LINE__);
            break;
        }

        /*Implement poll related functionality here*/
        if (poll(pfd, CDC_NUM_FDS, -1) < 0) {
            /*Poll errored out, treat it as a fatal error bail out*/
            int error = errno;
            /**
             * Continue polling if poll error is EINTR
             */
            if (error == EINTR)
                continue;

            AGM_LOGE("Poll failed error %s", strerror(error));
            break;
        }

        AGM_LOGI("Out of poll");
        if (pfd[0].revents & (POLLIN|POLLPRI)) {

            memset(cdc_dl.recv_buf, 0 , CDC_MAX_RECEIVE_BUF_SIZE);
            receive_size = read(cdc_dl.drv_fd, cdc_dl.recv_buf,
                                CDC_MAX_RECEIVE_BUF_SIZE);
            temp = (uint32_t *) cdc_dl.recv_buf;
            AGM_LOGI("recieved buffer %x %x %x %x size %d",
                        temp[0], temp[1], temp[2], temp[3], receive_size);

            if ((receive_size <= 0) || (receive_size > CDC_MAX_RECEIVE_BUF_SIZE)) {
                AGM_LOGE("%s:%d read failed %d", __func__, __LINE__, errno);
            } else {
                //handle command
                cmd_rsp = (cdc_response_t*)cdc_dl.recv_buf;
                cmd_rsp_size = sizeof(cdc_response_t) + cmd_rsp->resp_size;
                if (!cmd_rsp->resp_size || (cmd_rsp_size != receive_size)) {
                   if (!cmd_rsp->resp_size)
                        AGM_LOGE("%s:%d empty cmd response", __func__, __LINE__); 
                   else
                        AGM_LOGE("%s:%d read %d bytes & rsp_size %d bytes mismatch ", 
                                 __func__, __LINE__,
                            receive_size, cmd_rsp_size); 
                   continue;
                }

                rsp_opcode = cmd_rsp->response_opcode;
                if (rsp_opcode == CODEC_CMD_CODEC_STATUS) {
                   AGM_LOGE("%s:%d skip handling 0x%X codec status",
                           __func__, __LINE__, CODEC_CMD_CODEC_STATUS);
                   continue;
                }

                ep_rsp = (ep_cmd_response_t*)(cmd_rsp->resp_data);
                ep_obj = ep_obj_retrieve_from_pool(ep_rsp->epid);
                if (!ep_obj) {
                   AGM_LOGE("%s:%d skip response for ep_obj %d - obj not found ",
                        __func__, __LINE__, ep_rsp->epid);
                   continue;
                }
                AGM_LOGI("%s:%d retrieved ep_obj", __func__, __LINE__);

                pthread_mutex_lock(&ep_obj->lock);
                if (!ep_obj->cmd) {
                    AGM_LOGE("no active cmd found for ep_obj %d", ep_obj->ep_id);
                    pthread_mutex_unlock(&ep_obj->lock);
                    continue;
                }

                if (ep_obj->cmd->cmd_opcode != rsp_opcode) {
                    AGM_LOGE("Recieved unexpected rsp opcode %x, expected %x",
                                    rsp_opcode, ep_obj->cmd->cmd_opcode);
                    pthread_mutex_unlock(&ep_obj->lock);
                    continue;
                }

                AGM_LOGI("%s: process response for opcode 0x%X with ep_id %d\n",
                        __func__, rsp_opcode, ep_obj->ep_id);

                cmd_rsp = (cdc_response_t*)calloc(1, cmd_rsp_size);
                if (cmd_rsp == NULL) {
                    AGM_LOGE("No memory to create cmd_rsp\n");
                    pthread_mutex_unlock(&ep_obj->lock);
                    continue;
                }
                memcpy(cmd_rsp, cdc_dl.recv_buf, receive_size);
                if (ep_obj->rsp) {
                    AGM_LOGI("%s: pending response for opcode 0x%X with ep_id %d\n",
                        __func__, ep_obj->rsp->response_opcode, ep_obj->ep_id);
                    free(ep_obj->rsp);
                    ep_obj->rsp = NULL;
                }
                ep_obj->rsp = cmd_rsp;
                //signal the cmd thread
                pthread_cond_broadcast(&ep_obj->cond);
                pthread_mutex_unlock(&ep_obj->lock);
            }
        } else if (pfd[0].revents & (POLLERR|POLLHUP|POLLNVAL)) {
            /*
             *We should hit this case when we are trying to exit
             *dl layer will close the driver, which inturn should
             *unblock poll with an error mask;
             */
            AGM_LOGE("%s:%d Poll errored", __func__, __LINE__);
            continue;
        } else if (pfd[1].revents & (POLLIN|POLLPRI)) {
            break;
        }
    }
    AGM_LOGI("%s:%d Receiver thread exit", __func__, __LINE__);
    return NULL;
}


int32_t ep_obj_response_timedwait(struct ep_obj *ep_obj, int64_t timeout_in_nsec)
{
    int32_t rc = 0;
    struct timespec osal_ts;

    clock_gettime(CLOCK_MONOTONIC, &osal_ts);
    osal_ts.tv_sec += (timeout_in_nsec / 1000000000);
    osal_ts.tv_nsec += (timeout_in_nsec % 1000000000);

    if (osal_ts.tv_nsec >= 1000000000) {
        osal_ts.tv_sec += 1;
        osal_ts.tv_nsec -= 1000000000;
    }

    rc = pthread_cond_timedwait(&ep_obj->cond, &ep_obj->lock, &osal_ts);
    if (rc) {
        AGM_LOGE("%s: Failed to wait on signal, rc = %d\n", __func__, rc);
    }
    return rc;
}
