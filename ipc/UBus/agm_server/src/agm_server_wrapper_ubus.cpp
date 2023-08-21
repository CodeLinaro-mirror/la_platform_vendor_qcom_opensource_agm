/**
* * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* * SPDX-License-Identifier: BSD-3-Clause-Clear
* *
**/

#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <agm/agm_api.h>
#include <glib.h>
#include "libubus.h"
#include "inc/agm_server_wrapper_ubus.h"
#include "inc/agm-ubus-utils.h"
#include "utils.h"
#define UBUS_UNIX_SOCKET "/var/run/ubus/ubus.sock"

static struct ubus_context *ctx;
static struct ubus_subscriber agm_ubus_event;
static struct blob_buf b;
GHashTable *ses_hash_table;

typedef struct {
    struct ubus_context* ctx;
    uint32_t session_id;
}agm_client_session_data;

static int ipc_agm_session_get_buf_info(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_get_params(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_aif_set_cal(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_aif_set_params(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_aif_set_params(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_aif_get_tag_module_info_size(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_aif_get_tag_module_info(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_audio_inf_connect(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_set_ec_ref(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_deregister_cb(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_register_cb(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_register_for_events(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_set_params(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_get_aif_info_list(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_get_aif_info_list_size(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_set_params_with_tag(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_set_loopback(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_set_metadata(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_audio_intf_set_metadata(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_aif_set_media_config(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_get_buffer_timestamp(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_get_hw_processed_buff_cnt(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_get_session_time(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_eos(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg);
static int ipc_agm_session_set_config(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_write(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_read(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_pause(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg);
static int ipc_agm_session_resume(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg);
static int ipc_agm_session_prepare(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg);
static int ipc_agm_session_start(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg);
static int ipc_agm_session_stop(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg);
static int ipc_agm_session_close(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg);
static int ipc_agm_session_open(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);
static int ipc_agm_session_aif_set_metadata(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg);


enum {
    AGM_SESSION_GET_BUF_INFO_SESSION_ID,
    //AGM_SESSION_GET_BUF_INFO_BUF_INFO,
    AGM_SESSION_GET_BUF_INFO_FlAG,
    __AGM_SESSION_GET_BUF_INFO_MAX,
    AGM_SESSION_GET_PARAMS_SESSION_ID,
    AGM_SESSION_GET_PARAMS_SIZE,
    AGM_SESSION_GET_PARAMS_PAYLOAD,
    __AGM_SESSION_GET_PARAMS_MAX,
    AGM_SESSION_AIF_SET_CAL_AIF_ID,
    AGM_SESSION_AIF_SET_CAL_SESSION_ID,
    AGM_SESSION_AIF_SET_CAL_CAL_CONFIG,
    __AGM_SESSION_AIF_SET_CAL_MAX,
    AGM_AIF_SET_PARAMS_AIF_ID,
    AGM_AIF_SET_PARAMS_PAYLOAD,
    AGM_AIF_SET_PARAMS_SIZE,
    __AGM_AIF_SET_PARAMS_MAX,
    AGM_SESSION_AIF_SET_PARAMS_AIF_ID,
    AGM_SESSION_AIF_SET_PARAMS_SESSION_ID,
    AGM_SESSION_AIF_SET_PARAMS_PAYLOAD,
    AGM_SESSION_AIF_SET_PARAMS_SIZE,
    __AGM_SESSION_AIF_SET_PARAMS_MAX,
    AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SESSION_ID,
    AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SIZE,
    AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_AIF_ID,
    __AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_MAX,
    AGM_SESSION_AIF_GET_TAG_MODULE_INFO_AIF_ID,
    AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SESSION_ID,
    //AGM_SESSION_AIF_GET_TAG_MODULE_INFO_PAYLOAD,
    AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE,
    __AGM_SESSION_AIF_GET_TAG_MODULE_INFO_MAX,
    AGM_SESSION_AIF_CONNECT_SESSION_ID,
    AGM_SESSION_AIF_CONNECT_AIF_ID,
    AGM_SESSION_AIF_CONNECT_STATE,
    __AGM_SESSION_AIF_CONNECT_MAX,
    AGM_SESSION_SET_EC_REF_CAPTURE_SESSION_ID,
    AGM_SESSION_SET_EC_REF_AIF_ID,
    AGM_SESSION_SET_EC_REF_STATE,
    __AGM_SESSION_SET_EC_REF_MAX,
    AGM_SESSION_DEREGISTER_CB_SESSION_ID,
    AGM_SESSION_DEREGISTER_CB_CLIENT_DATA,
    AGM_SESSION_DEREGISTER_CB_EVENT_TYPE,
    __AGM_SESSION_DEREGISTER_CB_MAX,
    AGM_SESSION_REGISTER_CB_SESSION_ID,
    AGM_SESSION_REGISTER_CB_CLIENT_DATA,
    AGM_SESSION_REGISTER_CB_EVENT_TYPE,
    AGM_SESSION_REGISTER_CB_AGM_EVENT_CB,
    __AGM_SESSION_REGISTER_CB_MAX,
    AGM_SESSION_REGISTER_FOR_EVENTS_SESSION_ID,
    AGM_SESSION_REGISTER_FOR_EVENTS_REG_CFG,
    __AGM_SESSION_REGISTER_FOR_EVENTS_MAX,
    AGM_SESSION_SET_PARAMS_SESSION_ID,
    AGM_SESSION_SET_PARAMS_SIZE,
    AGM_SESSION_SET_PARAMS_PAYLOAD,
    __AGM_SESSION_SET_PARAMS_MAX,
    AGM_GET_AIF_INFO_LIST_AIF_INFO,
    //AGM_GET_AIF_INFO_LIST_AIF_LIST,
    __AGM_GET_AIF_INFO_LIST_MAX,
    //AGM_GET_AIF_INFO_LIST_SIZE_AIF_INFO,
    __AGM_GET_AIF_INFO_LIST_SIZE_MAX,
    AGM_SET_PARAMS_WITH_TAG_SESSION_ID,
    AGM_SET_PARAMS_WITH_TAG_AIF_ID,
    AGM_SET_PARAMS_WITH_TAG_TAG_CONFIG,
    __AGM_SET_PARAMS_WITH_TAG_MAX,
    AGM_SESSION_SET_LOOPBACK_CAPTURE_SESSION_ID,
    AGM_SESSION_SET_LOOPBACK_PLAYBACK_SESSION_ID,
    AGM_SESSION_SET_LOOPBACK_STATE,
    __AGM_SESSION_SET_LOOPBACK_MAX,
    AGM_SESSION_SET_METADATA_SESSION_ID,
    AGM_SESSION_SET_METADATA_SIZE,
    AGM_SESSION_SET_METADATA_METADATA,
    __AGM_SESSION_SET_METADATA_MAX,
    AGM_SESSION_AIF_SET_METADATA_AIF_ID,
    AGM_SESSION_AIF_SET_METADATA_SESSION_ID,
    AGM_SESSION_AIF_SET_METADATA_SIZE,
    AGM_SESSION_AIF_SET_METADATA_METADATA,
    __AGM_SESSION_AIF_SET_METADATA_MAX,
    AGM_AIF_SET_METADATA_AIF_ID,
    AGM_AIF_SET_METADATA_SIZE,
    AGM_AIF_SET_METADATA_METADATA,
    __AGM_AIF_SET_METADATA_MAX,
    AGM_AIF_SET_MEDIA_CONFIG_AIF_ID,
    AGM_AIF_SET_MEDIA_CONFIG_MEDIA_CONFIG,
    __AGM_AIF_SET_MEDIA_CONFIG_MAX,
    AGM_GET_BUFFER_TIMESTAMP_SESSION_ID,
    //AGM_GET_BUFFER_TIMESTAMP_TIMESTAMP,
    __AGM_GET_BUFFER_TIMESTAMP_MAX,
    AGM_GET_HW_PROCESSED_BUF_COUNT_DIRECTION,
    AGM_GET_HW_PROCESSED_BUF_COUNT_SESSION_HANDLE,
    __AGM_GET_HW_PROCESSED_BUF_COUNT_MAX,
    AGM_SESSION_GET_TIME_SESSION_HANDLE,
    //AGM_SESSION_GET_TIME_TIMESTAMP,
    __AGM_SESSION_GET_TIME_MAX,
    AGM_SESSION_EOS_SESSION_HANDLE,
    __AGM_SESSION_EOS_MAX,
    AGM_SESSION_SET_CONFIG_SESSION_HANDLE,
    AGM_SESSION_SET_CONFIG_SESSION_CONFIG,
    AGM_SESSION_SET_CONFIG_MEDIA_CONFIG,
    AGM_SESSION_SET_CONFIG_BUFFER_CONFIG,
    __AGM_SESSION_SET_CONFIG_MAX,
    AGM_SESSION_WRITE_SESSION_HANDLE,
    AGM_SESSION_WRITE_BUFFER,
    AGM_SESSION_WRITE_BYTE_COUNT,
    __AGM_SESSION_WRITE_MAX,
    AGM_SESSION_READ_SESSION_HANDLE,
    AGM_SESSION_READ_BUFFER,
    AGM_SESSION_READ_BYTE_COUNT,
    __AGM_SESSION_READ_MAX,
    AGM_SESSION_PAUSE_SESSION_HANDLE,
    __AGM_SESSION_PAUSE_MAX,
    AGM_SESSION_RESUME_SESSION_HANDLE,
    __AGM_SESSION_RESUME_MAX,
    AGM_SESSION_PREPARE_SESSION_HANDLE,
    __AGM_SESSION_PREPARE_MAX,
    AGM_SESSION_START_SESSION_HANDLE,
    __AGM_SESSION_START_MAX,
    AGM_SESSION_STOP_SESSION_HANDLE,
    __AGM_SESSION_STOP_MAX,
    AGM_SESSION_CLOSE_SESSION_HANDLE,
    __AGM_SESSION_CLOSE_MAX,
    AGM_SESSION_OPEN_SESSION_ID,
    AGM_SESSION_OPEN_SESSION_HANDLE,
    AGM_SESSION_OPEN_SESSION_MODE,
    __AGM_SESSION_OPEN_MAX
};


static struct blobmsg_policy AgmSessionGetBufInfo_policy[__AGM_SESSION_GET_BUF_INFO_MAX];
static struct blobmsg_policy AgmSessionGetParams_policy[__AGM_SESSION_GET_PARAMS_MAX];
static struct blobmsg_policy AgmSessionAifSetCal_policy[__AGM_SESSION_AIF_SET_CAL_MAX];
static struct blobmsg_policy AgmAifSetParams_policy[__AGM_AIF_SET_PARAMS_MAX];
static struct blobmsg_policy AgmSessionAifSetParams_policy[__AGM_SESSION_AIF_SET_PARAMS_MAX];
static struct blobmsg_policy AgmSessionAifGetTagModuleInfoSize_policy[__AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_MAX];
static struct blobmsg_policy AgmSessionAifGetTagModuleInfo_policy[__AGM_SESSION_AIF_GET_TAG_MODULE_INFO_MAX];
static struct blobmsg_policy AgmSessionAifConnect_policy[__AGM_SESSION_AIF_CONNECT_MAX];
static struct blobmsg_policy AgmSessionSetEcRef_policy[__AGM_SESSION_SET_EC_REF_MAX];
static struct blobmsg_policy AgmSessionDeregisterCb_policy[__AGM_SESSION_DEREGISTER_CB_MAX];
static struct blobmsg_policy AgmSessionRegisterCb_policy[__AGM_SESSION_REGISTER_CB_MAX];
static struct blobmsg_policy AgmSessionRegisterForEvents_policy[__AGM_SESSION_REGISTER_FOR_EVENTS_MAX];
static struct blobmsg_policy AgmSessionSetParams_policy[__AGM_SESSION_SET_PARAMS_MAX];
static struct blobmsg_policy AgmGetAifInfoList_policy[__AGM_GET_AIF_INFO_LIST_MAX];
static struct blobmsg_policy AgmGetAifInfoListSize_policy[__AGM_GET_AIF_INFO_LIST_SIZE_MAX];
static struct blobmsg_policy AgmSetParamsWithTag_policy[__AGM_SET_PARAMS_WITH_TAG_MAX];
static struct blobmsg_policy AgmSessionSetLoopback_policy[__AGM_SESSION_SET_LOOPBACK_MAX];
static struct blobmsg_policy AgmSessionSetMetadata_policy[__AGM_SESSION_SET_METADATA_MAX];
static struct blobmsg_policy AgmSessionAifSetMetadata_policy[__AGM_SESSION_AIF_SET_METADATA_MAX];
static struct blobmsg_policy AgmAifSetMetadata_policy[__AGM_AIF_SET_METADATA_MAX];
static struct blobmsg_policy AgmAifSetMediaConfig_policy[__AGM_AIF_SET_MEDIA_CONFIG_MAX];
static struct blobmsg_policy AgmGetBufferTimestamp_policy[__AGM_GET_BUFFER_TIMESTAMP_MAX];
static struct blobmsg_policy AgmGetHwProcessedBufCount_policy[__AGM_GET_HW_PROCESSED_BUF_COUNT_MAX];
static struct blobmsg_policy AgmSessionGetTime_policy[__AGM_SESSION_GET_TIME_MAX];
static struct blobmsg_policy AgmSessionEos_policy[__AGM_SESSION_EOS_MAX];
static struct blobmsg_policy AgmSessionSetConfig_policy[__AGM_SESSION_SET_CONFIG_MAX];
static struct blobmsg_policy AgmSessionWrite_policy[__AGM_SESSION_WRITE_MAX];
static struct blobmsg_policy AgmSessionRead_policy[__AGM_SESSION_READ_MAX];
static struct blobmsg_policy AgmSessionPause_policy[__AGM_SESSION_PAUSE_MAX];
static struct blobmsg_policy AgmSessionResume_policy[__AGM_SESSION_RESUME_MAX] ;
static struct blobmsg_policy AgmSessionPrepare_policy[__AGM_SESSION_PREPARE_MAX];
static struct blobmsg_policy AgmSessionStart_policy[__AGM_SESSION_START_MAX];
static struct blobmsg_policy AgmSessionStop_policy[__AGM_SESSION_STOP_MAX];
static struct blobmsg_policy AgmSessionClose_policy[__AGM_SESSION_CLOSE_MAX];
static struct blobmsg_policy  AgmSessionOpen_policy[__AGM_SESSION_OPEN_MAX];


static struct ubus_method agm_ubus_module_methods[] = {
    UBUS_METHOD("AgmAifSetMediaConfig", ipc_agm_aif_set_media_config, AgmAifSetMediaConfig_policy),
    UBUS_METHOD("AgmAifSetMetadata", ipc_agm_audio_intf_set_metadata, AgmAifSetMetadata_policy),
    UBUS_METHOD("AgmSessionAifSetMetadata", ipc_agm_session_aif_set_metadata, AgmSessionAifSetMetadata_policy),
    UBUS_METHOD("AgmSessionSetMetadata", ipc_agm_session_set_metadata, AgmSessionSetMetadata_policy),
    UBUS_METHOD("AgmSessionSetLoopback", ipc_agm_session_set_loopback, AgmSessionSetLoopback_policy),
    UBUS_METHOD("AgmSetParamsWithTag", ipc_agm_set_params_with_tag, AgmSetParamsWithTag_policy),
    UBUS_METHOD("AgmGetAifInfoListSize" , ipc_agm_get_aif_info_list_size, AgmGetAifInfoListSize_policy),
    UBUS_METHOD("AgmGetAifInfoList" , ipc_agm_get_aif_info_list, AgmGetAifInfoList_policy),
    UBUS_METHOD("AgmSessionSetParams", ipc_agm_session_set_params, AgmSessionSetParams_policy),
    UBUS_METHOD("AgmSessionRegisterForEvents", ipc_agm_session_register_for_events, AgmSessionRegisterForEvents_policy),
    UBUS_METHOD("AgmSessionRegisterCb", ipc_agm_session_register_cb, AgmSessionRegisterCb_policy),
    UBUS_METHOD("AgmSessionDeregisterCb", ipc_agm_session_deregister_cb, AgmSessionDeregisterCb_policy),
    UBUS_METHOD("AgmSessionSetEcRef", ipc_agm_session_set_ec_ref, AgmSessionSetEcRef_policy),
    UBUS_METHOD("AgmSessionAifConnect", ipc_agm_session_audio_inf_connect, AgmSessionAifConnect_policy),
    UBUS_METHOD("AgmSessionAifGetTagModuleInfo", ipc_agm_session_aif_get_tag_module_info, AgmSessionAifGetTagModuleInfo_policy),
    UBUS_METHOD("AgmSessionAifGetTagModuleInfoSize", ipc_agm_session_aif_get_tag_module_info_size, AgmSessionAifGetTagModuleInfoSize_policy),
    UBUS_METHOD("AgmSessionAifSetParams", ipc_agm_session_aif_set_params, AgmSessionAifSetParams_policy),
    UBUS_METHOD("AgmAifSetParams", ipc_agm_aif_set_params, AgmAifSetParams_policy),
    UBUS_METHOD("AgmSessionAifSetCal", ipc_agm_session_aif_set_cal, AgmSessionAifSetCal_policy),
    UBUS_METHOD("AgmSessionGetParams", ipc_agm_session_get_params, AgmSessionGetParams_policy),
    UBUS_METHOD("AgmSessionGetBufInfo", ipc_agm_session_get_buf_info, AgmSessionGetBufInfo_policy),
    UBUS_METHOD("AgmGetBufferTimestamp", ipc_agm_get_buffer_timestamp, AgmGetBufferTimestamp_policy),
    UBUS_METHOD("AgmSessionOpen", ipc_agm_session_open, AgmSessionOpen_policy),
};

static struct ubus_method agm_ubus_session_methods[] = {
    UBUS_METHOD("AgmSessionClose", ipc_agm_session_close, AgmSessionClose_policy),
    UBUS_METHOD("AgmSessionPrepare", ipc_agm_session_prepare, AgmSessionPrepare_policy),
    UBUS_METHOD("AgmSessionStart", ipc_agm_session_start, AgmSessionStart_policy),
    UBUS_METHOD("AgmSessionStop", ipc_agm_session_stop, AgmSessionStop_policy),
    UBUS_METHOD("AgmSessionResume", ipc_agm_session_resume, AgmSessionResume_policy),
    UBUS_METHOD("AgmSessionPause", ipc_agm_session_pause, AgmSessionPause_policy),
    UBUS_METHOD("AgmSessionWrite" , ipc_agm_session_write, AgmSessionWrite_policy),
    UBUS_METHOD("AgmSessionRead" , ipc_agm_session_read, AgmSessionRead_policy),
    UBUS_METHOD("AgmSessionSetConfig", ipc_agm_session_set_config, AgmSessionSetConfig_policy),
    UBUS_METHOD("AgmSessionEos", ipc_agm_session_eos, AgmSessionEos_policy),
    UBUS_METHOD("AgmSessionGetTime", ipc_agm_get_session_time, AgmSessionGetTime_policy),
    UBUS_METHOD("AgmGetHwProcessedBufCount", ipc_agm_get_hw_processed_buff_cnt, AgmGetHwProcessedBufCount_policy),
};

static struct ubus_object_type agm_ubus_module_object_type = {
    .name = "agm_ubus_module",
    .id = 0,
    .methods = agm_ubus_module_methods,
    .n_methods = ARRAY_SIZE(agm_ubus_module_methods)
};

static struct ubus_object_type agm_ubus_session_object_type = {
    .name = "agm_ubus_session",
    .id = 0,
    .methods = agm_ubus_session_methods,
    .n_methods = ARRAY_SIZE(agm_ubus_session_methods)
};

static struct ubus_object agm_ubus_module_object = {
    .name = "agm_ubus_module",
    .type = &agm_ubus_module_object_type,
    .methods = agm_ubus_module_methods,
    .n_methods = ARRAY_SIZE(agm_ubus_module_methods),
};

static struct ubus_object agm_ubus_session_object = {
    .name = "agm_ubus_session",
    .type = &agm_ubus_session_object_type,
    .methods = agm_ubus_session_methods,
    .n_methods = ARRAY_SIZE(agm_ubus_session_methods),
};

void agm_ubus_send_error(struct ubus_request_data *req, uint32_t err){
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_ubus_error", err);
    ubus_send_reply(ctx, req, b.head);
}

static int ipc_agm_session_get_buf_info(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    AGM_LOGE("%s\n", __func__);
    struct blob_attr *tb[__AGM_SESSION_GET_BUF_INFO_MAX];
    uint32_t session_id, flag;
    struct agm_buf_info *buf_info;

    AGM_LOGE("%s defining the return policies\n", __func__);
    AgmSessionGetBufInfo_policy[AGM_SESSION_GET_BUF_INFO_SESSION_ID].name = "AgmSessionGetBufInfo_session_id";
    AgmSessionGetBufInfo_policy[AGM_SESSION_GET_BUF_INFO_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    /*AgmSessionGetBufInfo_policy[AGM_SESSION_GET_BUF_INFO_BUF_INFO].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionGetBufInfo_policy[AGM_SESSION_GET_BUF_INFO_BUF_INFO].name = "AgmSessionGetBufInfo_buf_info";*/
    AgmSessionGetBufInfo_policy[AGM_SESSION_GET_BUF_INFO_FlAG].name = "AgmSessionGetBufInfo_flag";
    AgmSessionGetBufInfo_policy[AGM_SESSION_GET_BUF_INFO_FlAG].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Getting data from client\n", __func__);
    blobmsg_parse(AgmSessionGetBufInfo_policy, __AGM_SESSION_GET_BUF_INFO_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_SESSION_GET_BUF_INFO_SESSION_ID] || !tb[AGM_SESSION_GET_BUF_INFO_FlAG]){
        AGM_LOGE("%s Error in getting Data from Client\n", __func__);
        agm_ubus_send_error(req, UBUS_STATUS_INVALID_ARGUMENT);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    flag = blobmsg_get_u32(tb[AGM_SESSION_GET_BUF_INFO_FlAG]);
    session_id = blobmsg_get_u32(tb[AGM_SESSION_GET_BUF_INFO_SESSION_ID]);
    //buf_info = (struct agm_buf_info* )blobmsg_data(tb[AGM_SESSION_GET_BUF_INFO_BUF_INFO]);

    AGM_LOGE("%s Recieved Data from Client\n",__func__);

    if (agm_session_get_buf_info(session_id, buf_info, flag) != 0) {
        AGM_LOGE("agm_session_get_buf_info failed\n");
        free(buf_info);
        buf_info = NULL;
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("agm_session_get_buf_info success\n");
    AGM_LOGE("%s Replying to Client\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_get_buf_info_session_id", session_id);
    blobmsg_add_u32(&b, "agm_session_get_buf_info_flag", flag);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_get_buf_info_buf_info", buf_info, sizeof(*buf_info));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s data sent successfully\n", __func__);
    return 0;
}

static int ipc_agm_session_get_params(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_GET_PARAMS_MAX];
    uint32_t session_id;
    void* payload;
    size_t* size;

    AGM_LOGE("%s Defining the return polocies\n", __func__);
    AgmSessionGetParams_policy[AGM_SESSION_GET_PARAMS_SESSION_ID].name = "AgmSessionGetParams_session_id";
    AgmSessionGetParams_policy[AGM_SESSION_GET_PARAMS_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionGetParams_policy[AGM_SESSION_GET_PARAMS_SIZE].name = "AgmSessionGetParams_payload";
    AgmSessionGetParams_policy[AGM_SESSION_GET_PARAMS_SIZE].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionGetParams_policy[AGM_SESSION_GET_PARAMS_PAYLOAD].name = "AgmSessionGetParams_size";
    AgmSessionGetParams_policy[AGM_SESSION_GET_PARAMS_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s getting the data from Client\n", __func__);
    blobmsg_parse(AgmSessionGetParams_policy, __AGM_SESSION_GET_PARAMS_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_SESSION_GET_PARAMS_SESSION_ID] || !tb[AGM_SESSION_GET_PARAMS_SIZE] || !tb[AGM_SESSION_GET_PARAMS_PAYLOAD]) {
        agm_ubus_send_error(req, UBUS_STATUS_INVALID_ARGUMENT);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    payload = blobmsg_data(tb[AGM_SESSION_GET_PARAMS_PAYLOAD]);
    session_id = blobmsg_get_u32(tb[AGM_SESSION_GET_PARAMS_SESSION_ID]);
    size = (size_t* )blobmsg_data(tb[AGM_SESSION_GET_PARAMS_SIZE]);

    AGM_LOGE("calling agm_session_get_params\n");
    if (agm_session_get_params(session_id, (void *)payload, *size) != 0) {
        AGM_LOGE("agm_session_get_params failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        free(payload);
        payload = NULL;
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to client\n", __func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_get_params_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_get_params_payload", payload, sizeof(payload));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_get_params_size", size, sizeof(*size));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s reply sent successfully", __func__);

    return 0;
}

static int ipc_agm_session_aif_set_cal(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_AIF_SET_CAL_MAX];
    uint32_t session_id, aif_id;
    struct agm_cal_config *cal_config;

    AGM_LOGE("%s Defining the return policies\n", __func__);
    AgmSessionAifSetCal_policy[AGM_SESSION_AIF_SET_CAL_SESSION_ID].name = "AgmSessionAifSetCal_session_id";
    AgmSessionAifSetCal_policy[AGM_SESSION_AIF_SET_CAL_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionAifSetCal_policy[AGM_SESSION_AIF_SET_CAL_AIF_ID].name = "AgmSessionAifSetCal_aif_id";
    AgmSessionAifSetCal_policy[AGM_SESSION_AIF_SET_CAL_AIF_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionAifSetCal_policy[AGM_SESSION_AIF_SET_CAL_CAL_CONFIG].name = "AgmSessionAifSetCal_cal_config";
    AgmSessionAifSetCal_policy[AGM_SESSION_AIF_SET_CAL_CAL_CONFIG].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n", __func__);
    blobmsg_parse(AgmSessionAifSetCal_policy, __AGM_SESSION_AIF_SET_CAL_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_SESSION_AIF_SET_CAL_AIF_ID] || !tb[AGM_SESSION_AIF_SET_CAL_SESSION_ID] || !tb[AGM_SESSION_AIF_SET_CAL_CAL_CONFIG]){
        AGM_LOGE("%s Error in Recieving data from client", __func__);
        agm_ubus_send_error(req, UBUS_STATUS_INVALID_ARGUMENT);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_SET_CAL_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_SET_CAL_AIF_ID]);
    cal_config = (struct agm_cal_config*)blobmsg_data(tb[AGM_SESSION_AIF_SET_CAL_CAL_CONFIG]);

    AGM_LOGE("Calling agm_session_set_cal\n");
    if (agm_session_aif_set_cal(session_id, aif_id, cal_config)) {
        AGM_LOGE("%s:agm_session_aif_set_cal failed.\n", __func__);
        free(cal_config);
        cal_config = NULL;
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to client\n", __func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_aif_set_cal_session_id", session_id);
    blobmsg_add_u32(&b, "agm_session_aif_set_cal_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_aif_set_cal_cal_config", cal_config, sizeof(&cal_config));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply Sent to client\n", __func__);

    return 0;
}

static int ipc_agm_aif_set_params(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method, struct blob_attr *msg){
    struct blob_attr *tb[__AGM_AIF_SET_PARAMS_MAX];
    uint32_t aif_id;
    void *payload;
    size_t *size;

    AGM_LOGE("%s Defining the return polocies\n", __func__);
    AgmAifSetParams_policy[AGM_AIF_SET_PARAMS_AIF_ID].name = "AgmAifSetParams_aif_id";
    AgmAifSetParams_policy[AGM_AIF_SET_PARAMS_AIF_ID].type = BLOBMSG_TYPE_INT32;
    AgmAifSetParams_policy[AGM_AIF_SET_PARAMS_PAYLOAD].name = "AgmAifSetParams_payload";
    AgmAifSetParams_policy[AGM_AIF_SET_PARAMS_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;
    AgmAifSetParams_policy[AGM_AIF_SET_PARAMS_SIZE].name = "AgmAifSetParams_size";
    AgmAifSetParams_policy[AGM_AIF_SET_PARAMS_SIZE].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n", __func__);
    blobmsg_parse(AgmAifSetParams_policy, __AGM_AIF_SET_PARAMS_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_AIF_SET_PARAMS_AIF_ID] || !tb[AGM_AIF_SET_PARAMS_PAYLOAD] || !tb[AGM_AIF_SET_PARAMS_SIZE]){
        AGM_LOGE("%s Error in Getting data From client\n", __func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    aif_id = blobmsg_get_u32(tb[AGM_AIF_SET_PARAMS_AIF_ID]);
    payload = blobmsg_data(tb[AGM_AIF_SET_PARAMS_PAYLOAD]);
    size = (size_t*)blobmsg_data(tb[AGM_AIF_SET_PARAMS_SIZE]);

    AGM_LOGE("Calling agm_aif_set_params\n");
    if (agm_aif_set_params(aif_id, payload, *size) != 0) {
        AGM_LOGE("agm_aif_set_params failed.");
        free(payload);
        payload = NULL;
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to client\n", __func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_aif_set_params_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_aif_set_params_payload", payload, sizeof(&payload));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_aif_set_params_size", size, sizeof(&size));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply Sent to client\n", __func__);
    return 0;
}

static int ipc_agm_session_aif_set_params(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_AIF_SET_PARAMS_MAX];
    uint32_t session_id, aif_id;
    void *payload;
    size_t *size;

    AGM_LOGE("%s Defining the retutrn policies\n", __func__);
    AgmSessionAifSetParams_policy[AGM_SESSION_AIF_SET_PARAMS_SESSION_ID].name = "AgmSessionAifSetParams_aif_id";
    AgmSessionAifSetParams_policy[AGM_SESSION_AIF_SET_PARAMS_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionAifSetParams_policy[AGM_SESSION_AIF_SET_PARAMS_AIF_ID].name = "AgmSessionAifSetParams_aif_id";
    AgmSessionAifSetParams_policy[AGM_SESSION_AIF_SET_PARAMS_AIF_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionAifSetParams_policy[AGM_SESSION_AIF_SET_PARAMS_PAYLOAD].name = "AgmSessionAifSetParams_payload";
    AgmSessionAifSetParams_policy[AGM_SESSION_AIF_SET_PARAMS_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionAifSetParams_policy[AGM_SESSION_AIF_SET_PARAMS_SIZE].name = "AgmSessionAifSetParams_size";
    AgmSessionAifSetParams_policy[AGM_SESSION_AIF_SET_PARAMS_SIZE].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionAifSetParams_policy, __AGM_SESSION_AIF_SET_PARAMS_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_SESSION_AIF_SET_PARAMS_AIF_ID] || !tb[AGM_SESSION_AIF_SET_PARAMS_SESSION_ID] ||
                !tb[AGM_SESSION_AIF_SET_PARAMS_PAYLOAD] || !tb[AGM_SESSION_AIF_SET_PARAMS_SIZE]) {
        AGM_LOGE("%s Error in Receiving the data from Client\n", __func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_SET_PARAMS_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_SET_PARAMS_AIF_ID]);
    payload = blobmsg_data(tb[AGM_SESSION_AIF_SET_PARAMS_PAYLOAD]);
    size = (size_t*)blobmsg_data(tb[AGM_SESSION_AIF_SET_PARAMS_SIZE]);

    AGM_LOGE("Calling agm_session_aif_set_params\n");
    if (agm_session_aif_set_params(session_id, aif_id, payload, *size) != 0) {
        AGM_LOGE("agm_session_aif_set_params failed\n");
        free(payload);
        payload = NULL;
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_aif_set_params_session_id", session_id);
    blobmsg_add_u32(&b, "agm_session_aif_set_params_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_aif_set_params_payload", payload, sizeof(&payload));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_aif_set_params_size", size, sizeof(&size));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

static int ipc_agm_session_aif_get_tag_module_info_size(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_MAX];
    uint32_t session_id, aif_id;
    size_t *size;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmSessionAifGetTagModuleInfoSize_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SESSION_ID].name =  "AgmSessionAifGetTagModuleInfoSize_session_id";
    AgmSessionAifGetTagModuleInfoSize_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SESSION_ID].type =  BLOBMSG_TYPE_INT32;
    AgmSessionAifGetTagModuleInfoSize_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_AIF_ID].name =  "AgmSessionAifGetTagModuleInfoSize_aif_id";
    AgmSessionAifGetTagModuleInfoSize_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_AIF_ID].type =  BLOBMSG_TYPE_INT32;
    AgmSessionAifGetTagModuleInfoSize_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SIZE].name =  "AgmSessionAifGetTagModuleInfoSize_size";
    AgmSessionAifGetTagModuleInfoSize_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SIZE].type =  BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionAifGetTagModuleInfoSize_policy, __AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_AIF_ID] || !tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SESSION_ID] || !tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SIZE]) {
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_AIF_ID]);
    size = (size_t*)blobmsg_data(tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE_SIZE]);

    AGM_LOGE("Calling agm_session_aif_get_tag_module_info\n");
    if (agm_session_aif_get_tag_module_info(session_id,
                                            aif_id,
                                            NULL,
                                            size) != 0) {
        AGM_LOGE("agm_session_aif_get_tag_module_info failed\n");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_aif_get_tag_module_info_size_session_id", session_id);
    blobmsg_add_u32(&b, "agm_session_aif_get_tag_module_info_size_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_aif_get_tag_module_info_size_size", size, sizeof(&size));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

static int ipc_agm_session_aif_get_tag_module_info(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_AIF_GET_TAG_MODULE_INFO_MAX];
    uint32_t session_id, aif_id;
    void *payload;
    size_t *size;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmSessionAifGetTagModuleInfo_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SESSION_ID].name = "AgmSessionAifGetTagModuleInfo_session_id";
    AgmSessionAifGetTagModuleInfo_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionAifGetTagModuleInfo_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_AIF_ID].name = "AgmSessionAifGetTagModuleInfo_aif_id";
    AgmSessionAifGetTagModuleInfo_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_AIF_ID].type = BLOBMSG_TYPE_INT32;
    /*AgmSessionAifGetTagModuleInfo_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_PAYLOAD].name = "AgmSessionAifGetTagModuleInfo_payload";
    AgmSessionAifGetTagModuleInfo_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;*/
    AgmSessionAifGetTagModuleInfo_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE].name = "AgmSessionAifGetTagModuleInfo_size";
    AgmSessionAifGetTagModuleInfo_policy[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionAifGetTagModuleInfo_policy, __AGM_SESSION_AIF_GET_TAG_MODULE_INFO_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_AIF_ID] || !tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SESSION_ID] || !tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE]){
        AGM_LOGE("%s Error in Getting data from Client\n", __func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_AIF_ID]);
    //payload = blobmsg_data(tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_PAYLOAD]);
    size = (size_t*)blobmsg_data(tb[AGM_SESSION_AIF_GET_TAG_MODULE_INFO_SIZE]);

    AGM_LOGE("Calling agm_session_aif_get_tag_module_info\n");
    if (agm_session_aif_get_tag_module_info(session_id,
                                            aif_id,
                                            payload,
                                            size) != 0) {
        AGM_LOGE("agm_session_aif_get_tag_module_info failed.");
        free(payload);
        payload = NULL;
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_aif_get_tag_module_info_session_id", session_id);
    blobmsg_add_u32(&b, "agm_session_aif_get_tag_module_info_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_aif_get_tag_module_info_payload", payload, sizeof(&payload));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_aif_get_tag_module_info_size", size, sizeof(&size));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

static int ipc_agm_session_audio_inf_connect(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_AIF_CONNECT_MAX];
    uint32_t session_id, aif_id;
    bool state;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmSessionAifConnect_policy[AGM_SESSION_AIF_CONNECT_SESSION_ID].name = "AgmSessionAifConnect_session_id";
    AgmSessionAifConnect_policy[AGM_SESSION_AIF_CONNECT_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionAifConnect_policy[AGM_SESSION_AIF_CONNECT_AIF_ID].name = "AgmSessionAifConnect_aif_id";
    AgmSessionAifConnect_policy[AGM_SESSION_AIF_CONNECT_AIF_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionAifConnect_policy[AGM_SESSION_AIF_CONNECT_STATE].name = "AgmSessionAifConnect_state";
    AgmSessionAifConnect_policy[AGM_SESSION_AIF_CONNECT_STATE].type = BLOBMSG_TYPE_BOOL;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionAifConnect_policy, __AGM_SESSION_AIF_CONNECT_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_AIF_CONNECT_AIF_ID] || !tb[AGM_SESSION_AIF_CONNECT_SESSION_ID] || !tb[AGM_SESSION_AIF_CONNECT_STATE]) {
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_CONNECT_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_CONNECT_AIF_ID]);
    state = blobmsg_get_bool(tb[AGM_SESSION_AIF_CONNECT_STATE]);

    AGM_LOGE("Calling agm_session_aif_connect");
    if (agm_session_aif_connect(session_id, aif_id, (bool)state) != 0) {
        AGM_LOGE("%s :agm_session_aif_connect failed.", __func__);
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_aif_connect_session_id", session_id);
    blobmsg_add_u32(&b, "agm_session_aif_connect_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_BOOL, "agm_session_aif_connect_state", &state, sizeof(state));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

static int ipc_agm_session_set_ec_ref(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[AGM_SESSION_SET_EC_REF_AIF_ID];
    uint32_t capture_session_id, aif_id;
    bool state;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmSessionSetEcRef_policy[AGM_SESSION_SET_EC_REF_CAPTURE_SESSION_ID].name = "AgmSessionSetEcRef_capture_session_id";
    AgmSessionSetEcRef_policy[AGM_SESSION_SET_EC_REF_CAPTURE_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionSetEcRef_policy[AGM_SESSION_SET_EC_REF_AIF_ID].name = "AgmSessionSetEcRef_aif_id";
    AgmSessionSetEcRef_policy[AGM_SESSION_SET_EC_REF_AIF_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionSetEcRef_policy[AGM_SESSION_SET_EC_REF_STATE].name = "AgmSessionSetEcRef_state";
    AgmSessionSetEcRef_policy[AGM_SESSION_SET_EC_REF_STATE].type = BLOBMSG_TYPE_BOOL;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionSetEcRef_policy, __AGM_SESSION_SET_EC_REF_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_SET_EC_REF_AIF_ID] || !tb[AGM_SESSION_SET_EC_REF_CAPTURE_SESSION_ID] || !tb[AGM_SESSION_SET_EC_REF_STATE]){
        AGM_LOGE("%s Error in  Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    aif_id = blobmsg_get_u32(tb[AGM_SESSION_SET_EC_REF_AIF_ID]);
    capture_session_id = blobmsg_get_u32(tb[AGM_SESSION_SET_EC_REF_CAPTURE_SESSION_ID]);
    state = blobmsg_get_bool(tb[AGM_SESSION_SET_EC_REF_STATE]);

    AGM_LOGE("calling agm_session_set_ec_ref\n");
    if (agm_session_set_ec_ref(capture_session_id, aif_id, (bool)state) != 0) {
        AGM_LOGE("agm_session_set_ec_ref failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%sReplying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_set_ec_ref_capture_session_id", capture_session_id);
    blobmsg_add_u32(&b, "agm_session_set_ec_ref_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_BOOL, "agm_session_set_ec_ref_state", &state, sizeof(state));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

static int ipc_agm_session_deregister_cb(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_DEREGISTER_CB_MAX];
    uint32_t session_id;
    enum event_type* evt_type;
    void *client_data;
    agm_client_session_data *ses_data = NULL;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmSessionDeregisterCb_policy[AGM_SESSION_DEREGISTER_CB_SESSION_ID].name = "AgmSessionDeregisterCb_session_id";
    AgmSessionDeregisterCb_policy[AGM_SESSION_DEREGISTER_CB_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionDeregisterCb_policy[AGM_SESSION_DEREGISTER_CB_EVENT_TYPE].name = "AgmSessionDeregisterCb_event_type";
    AgmSessionDeregisterCb_policy[AGM_SESSION_DEREGISTER_CB_EVENT_TYPE].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionDeregisterCb_policy[AGM_SESSION_DEREGISTER_CB_CLIENT_DATA].name = "AgmSessionDeregisterCb_client_data";
    AgmSessionDeregisterCb_policy[AGM_SESSION_DEREGISTER_CB_CLIENT_DATA].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionDeregisterCb_policy, __AGM_SESSION_DEREGISTER_CB_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_DEREGISTER_CB_SESSION_ID] ||    !tb[AGM_SESSION_DEREGISTER_CB_EVENT_TYPE] || !tb[AGM_SESSION_DEREGISTER_CB_CLIENT_DATA]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_DEREGISTER_CB_SESSION_ID]);
    evt_type = (enum event_type*)blobmsg_data(tb[AGM_SESSION_DEREGISTER_CB_EVENT_TYPE]);
    client_data = blobmsg_data(tb[AGM_SESSION_DEREGISTER_CB_CLIENT_DATA]);

    AGM_LOGE("%s Hash Table\n",__func__);
    if((agm_client_session_data*)g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id)) == NULL ) {
        ses_data = (agm_client_session_data*)g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id));
        ses_data = (agm_client_session_data *)g_malloc0(sizeof(agm_client_session_data));
        ses_data -> session_id = session_id;
        ses_data -> ctx = ctx;
        g_hash_table_insert(ses_hash_table, GINT_TO_POINTER(session_id), ses_data);
    }

    AGM_LOGE("Calling agm_session_register_cb\n");
    if (agm_session_register_cb(session_id, NULL,
                                *evt_type,
                                (void *)ses_data) != 0) {
        AGM_LOGE("agm_session_register_cb failed.");
        //agm_free_session(ses_data);
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_deregister_cb_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_deregister_cb_event_type", &evt_type, sizeof(evt_type));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_deregister_cb_client_data", client_data, 0);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

static int ipc_agm_session_register_cb(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_REGISTER_CB_MAX];
    uint32_t session_id;
    agm_event_cb* cb;
    enum event_type* evt_type;
    void *client_data;
    agm_client_session_data *ses_data = NULL;

    AGM_LOGE("%s Defining the return policies\n", __func__);
    AgmSessionRegisterCb_policy[AGM_SESSION_REGISTER_CB_SESSION_ID].name = "AgmSessionRegisterCb_session_id";
    AgmSessionRegisterCb_policy[AGM_SESSION_REGISTER_CB_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionRegisterCb_policy[AGM_SESSION_REGISTER_CB_AGM_EVENT_CB].name = "AgmSessionRegisterCb_agm_event_cb";
    AgmSessionRegisterCb_policy[AGM_SESSION_REGISTER_CB_AGM_EVENT_CB].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionRegisterCb_policy[AGM_SESSION_REGISTER_CB_EVENT_TYPE].name = "AgmSessionRegisterCb_event_type";
    AgmSessionRegisterCb_policy[AGM_SESSION_REGISTER_CB_EVENT_TYPE].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionRegisterCb_policy[AGM_SESSION_REGISTER_CB_CLIENT_DATA].name = "AgmSessionRegisterCb_client_data";
    AgmSessionRegisterCb_policy[AGM_SESSION_REGISTER_CB_CLIENT_DATA].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionRegisterCb_policy, __AGM_SESSION_REGISTER_CB_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_REGISTER_CB_SESSION_ID] || !tb[AGM_SESSION_REGISTER_CB_AGM_EVENT_CB] ||
            !tb[AGM_SESSION_REGISTER_CB_EVENT_TYPE] || !tb[AGM_SESSION_REGISTER_CB_CLIENT_DATA]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_REGISTER_CB_SESSION_ID]);
    cb = (agm_event_cb* )blobmsg_data(tb[AGM_SESSION_REGISTER_CB_AGM_EVENT_CB]);
    evt_type = (enum event_type*)blobmsg_data(tb[AGM_SESSION_REGISTER_CB_EVENT_TYPE]);
    client_data = blobmsg_data(tb[AGM_SESSION_REGISTER_CB_CLIENT_DATA]);

    AGM_LOGE("%s Hash table\n", __func__);
    if((agm_client_session_data*)g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id)) == NULL ) {
        ses_data = (agm_client_session_data*)g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id));
        ses_data = (agm_client_session_data *)g_malloc0(sizeof(agm_client_session_data));
        ses_data -> session_id = session_id;
        ses_data -> ctx = ctx;
        g_hash_table_insert(ses_hash_table, GINT_TO_POINTER(session_id), ses_data);
    }

    AGM_LOGE("calling agm_session_register_cb\n");
    if (agm_session_register_cb(session_id,
                                *cb,
                                *evt_type,
                                (void *)ses_data) != 0) {
        AGM_LOGE("agm_session_register_cb failed.");
        //agm_free_session(ses_data);
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_register_cb_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_register_cb_agm_event_cb", &cb, sizeof(cb));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_register_cb_event_type", &evt_type, sizeof(evt_type));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_register_cb_client_data", client_data, 0);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

static int ipc_agm_session_register_for_events(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_REGISTER_FOR_EVENTS_MAX];
    uint32_t session_id;
    struct agm_event_reg_cfg *evt_reg_cfg;

    AGM_LOGE("%s Defining the return policies\n", __func__);
    AgmSessionRegisterForEvents_policy[AGM_SESSION_REGISTER_FOR_EVENTS_SESSION_ID].name = "AgmSessionRegisterForEvents_session_id";
    AgmSessionRegisterForEvents_policy[AGM_SESSION_REGISTER_FOR_EVENTS_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionRegisterForEvents_policy[AGM_SESSION_REGISTER_FOR_EVENTS_REG_CFG].name = "AgmSessionRegisterForEvents_reg_cfg";
    AgmSessionRegisterForEvents_policy[AGM_SESSION_REGISTER_FOR_EVENTS_REG_CFG].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionRegisterForEvents_policy, __AGM_SESSION_REGISTER_FOR_EVENTS_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_SESSION_REGISTER_FOR_EVENTS_SESSION_ID] || !tb[AGM_SESSION_REGISTER_FOR_EVENTS_REG_CFG]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_REGISTER_FOR_EVENTS_SESSION_ID]);
    evt_reg_cfg = (struct agm_event_reg_cfg* )blobmsg_data(tb[AGM_SESSION_REGISTER_FOR_EVENTS_REG_CFG]);

    AGM_LOGE("Calling agm_session_register_for_events\n");
    if (agm_session_register_for_events(session_id, evt_reg_cfg) != 0) {
        AGM_LOGE("agm_session_register_for_events failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_register_for_events_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_register_for_events_reg_cfg", evt_reg_cfg, sizeof(*evt_reg_cfg));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

static int ipc_agm_session_set_params(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_SET_PARAMS_MAX];
    uint32_t session_id;
    void* payload;
    size_t* size;

    AGM_LOGE("%s Defining the return policies\n", __func__);
    AgmSessionSetParams_policy[AGM_SESSION_SET_PARAMS_SESSION_ID].name = "AgmSessionSetParams_session_id";
    AgmSessionSetParams_policy[AGM_SESSION_SET_PARAMS_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionSetParams_policy[AGM_SESSION_SET_PARAMS_PAYLOAD].name = "AgmSessionSetParams_payload";
    AgmSessionSetParams_policy[AGM_SESSION_SET_PARAMS_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionSetParams_policy[AGM_SESSION_SET_PARAMS_SIZE].name = "AgmSessionSetParams_size";
    AgmSessionSetParams_policy[AGM_SESSION_SET_PARAMS_SIZE].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionSetParams_policy, __AGM_SESSION_SET_PARAMS_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_SESSION_SET_PARAMS_SESSION_ID] || !tb[AGM_SESSION_SET_PARAMS_SIZE] || !tb[AGM_SESSION_SET_PARAMS_PAYLOAD]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    payload = blobmsg_data(tb[AGM_SESSION_SET_PARAMS_PAYLOAD]);
    session_id = blobmsg_get_u32(tb[AGM_SESSION_SET_PARAMS_SESSION_ID]);
    size = (size_t* )blobmsg_data(tb[AGM_SESSION_SET_PARAMS_SIZE]);

    AGM_LOGE("Calling agm_session_set_params\n");
    if (agm_session_set_params(session_id, (void *)payload, *size) != 0) {
        AGM_LOGE("agm_session_set_params failed.");
        free(payload);
        payload = NULL;
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_set_params_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_set_params_payload", payload, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_set_params_size", size, sizeof(*size));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%sReply sent to Client\n",__func__);
    return 0;
}

// ------------ ipc_agm_get_aif_info_list -------------
static int ipc_agm_get_aif_info_list(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_GET_AIF_INFO_LIST_MAX];
    size_t *num_aif_info;
    struct aif_info *aif_list;

    AGM_LOGE("%s Defining the return policies\n", __func__);
    AgmGetAifInfoList_policy[AGM_GET_AIF_INFO_LIST_AIF_INFO].name = "AgmGetAifInfoList_aif_info";
    AgmGetAifInfoList_policy[AGM_GET_AIF_INFO_LIST_AIF_INFO].type = BLOBMSG_TYPE_UNSPEC;
    /*AgmGetAifInfoList_policy[AGM_GET_AIF_INFO_LIST_AIF_LIST].name = "AgmGetAifInfoList_aif_list";
    AgmGetAifInfoList_policy[AGM_GET_AIF_INFO_LIST_AIF_LIST].type = BLOBMSG_TYPE_UNSPEC;*/

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmGetAifInfoList_policy, __AGM_GET_AIF_INFO_LIST_MAX, tb, blob_data(msg), blob_len(msg));

    if(!tb[AGM_GET_AIF_INFO_LIST_AIF_INFO]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    num_aif_info = (size_t*) blobmsg_data(tb[AGM_GET_AIF_INFO_LIST_AIF_INFO]);
    //aif_list = (struct aif_info*)blobmsg_data(tb[AGM_GET_AIF_INFO_LIST_AIF_LIST]);

    AGM_LOGE("Calling agm_get_aif_info_list\n");
    if (agm_get_aif_info_list(aif_list, num_aif_info)) {
        AGM_LOGE("agm_get_aif_info_list failed");
        free(aif_list);
        aif_list = NULL;
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%sReplying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_get_aif_info_list_aif_info", num_aif_info, sizeof(*num_aif_info));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_get_aif_info_list_aif_list", aif_list, sizeof(*aif_list));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ---------------- ipc_agm_get_aif_info_list_size ------------------------
static int ipc_agm_get_aif_info_list_size(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_GET_AIF_INFO_LIST_SIZE_MAX];
    size_t *num_aif_info = 0;

    /*AgmGetAifInfoListSize_policy[AGM_GET_AIF_INFO_LIST_SIZE_AIF_INFO].name = "AgmGetAifInfoListSize_aif_info";
    AgmGetAifInfoListSize_policy[AGM_GET_AIF_INFO_LIST_SIZE_AIF_INFO].type = BLOBMSG_TYPE_UNSPEC;

    blobmsg_parse(AgmGetAifInfoListSize_policy, __AGM_GET_AIF_INFO_LIST_SIZE_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_GET_AIF_INFO_LIST_SIZE_AIF_INFO])
        return UBUS_STATUS_INVALID_ARGUMENT;

    num_aif_info = (size_t*) blobmsg_data(tb[AGM_GET_AIF_INFO_LIST_SIZE_AIF_INFO]);*/

    AGM_LOGE("Calling agm_get_aif_info_list\n");
    if (agm_get_aif_info_list(NULL, num_aif_info) != 0) {
        AGM_LOGE("agm_get_aif_info_list failed");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_get_aif_info_list_size_aif_info", num_aif_info, sizeof(*num_aif_info));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ---------------- ipc_agm_set_params_with_tag ---------------
static int ipc_agm_set_params_with_tag(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SET_PARAMS_WITH_TAG_MAX];
    uint32_t session_id, aif_id;
    struct agm_tag_config *tag_config;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmSetParamsWithTag_policy[AGM_SET_PARAMS_WITH_TAG_SESSION_ID].name = "AgmSetParamsWithTag_session_id";
    AgmSetParamsWithTag_policy[AGM_SET_PARAMS_WITH_TAG_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSetParamsWithTag_policy[AGM_SET_PARAMS_WITH_TAG_AIF_ID].name = "AgmSetParamsWithTag_aif_id";
    AgmSetParamsWithTag_policy[AGM_SET_PARAMS_WITH_TAG_AIF_ID].type = BLOBMSG_TYPE_INT32;
    AgmSetParamsWithTag_policy[AGM_SET_PARAMS_WITH_TAG_TAG_CONFIG].name = "AgmSetParamsWithTag_tag_config";
    AgmSetParamsWithTag_policy[AGM_SET_PARAMS_WITH_TAG_TAG_CONFIG].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSetParamsWithTag_policy, __AGM_SET_PARAMS_WITH_TAG_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SET_PARAMS_WITH_TAG_SESSION_ID] || !tb[AGM_SET_PARAMS_WITH_TAG_AIF_ID] || !tb[AGM_SET_PARAMS_WITH_TAG_TAG_CONFIG]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SET_PARAMS_WITH_TAG_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[AGM_SET_PARAMS_WITH_TAG_AIF_ID]);
    tag_config = (struct agm_tag_config*)blobmsg_data(tb[AGM_SET_PARAMS_WITH_TAG_TAG_CONFIG]);

    AGM_LOGE("calling agm_set_params_with_tag\n");
    if (agm_set_params_with_tag(session_id, aif_id, tag_config)) {
        AGM_LOGE("agm_set_params_with_tag failed.");
        free(tag_config);
        tag_config = NULL;
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_set_params_with_tag_session_id", session_id);
    blobmsg_add_u32(&b, "agm_set_params_with_tag_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_set_params_with_tag_tag_config", tag_config, sizeof(*tag_config));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ----------- ipc_agm_session_set_loopback ------------
static int ipc_agm_session_set_loopback(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_SET_LOOPBACK_MAX];
    uint32_t capture_session_id, playback_session_id;
    bool state;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmSessionSetLoopback_policy[AGM_SESSION_SET_LOOPBACK_CAPTURE_SESSION_ID].name = "AgmSessionSetLoopback_capture_session_id";
    AgmSessionSetLoopback_policy[AGM_SESSION_SET_LOOPBACK_CAPTURE_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionSetLoopback_policy[AGM_SESSION_SET_LOOPBACK_PLAYBACK_SESSION_ID].name = "AgmSessionSetLoopback_playback_session_id";
    AgmSessionSetLoopback_policy[AGM_SESSION_SET_LOOPBACK_PLAYBACK_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionSetLoopback_policy[AGM_SESSION_SET_LOOPBACK_STATE].name = "AgmSessionSetLoopback_state";
    AgmSessionSetLoopback_policy[AGM_SESSION_SET_LOOPBACK_STATE].type = BLOBMSG_TYPE_BOOL;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionSetLoopback_policy, __AGM_SESSION_SET_LOOPBACK_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_SET_LOOPBACK_CAPTURE_SESSION_ID] || !tb[AGM_SESSION_SET_LOOPBACK_PLAYBACK_SESSION_ID] || !tb[AGM_SESSION_SET_LOOPBACK_STATE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    capture_session_id = blobmsg_get_u32(tb[AGM_SESSION_SET_LOOPBACK_CAPTURE_SESSION_ID]);
    playback_session_id = blobmsg_get_u32(tb[AGM_SESSION_SET_LOOPBACK_PLAYBACK_SESSION_ID]);
    state = blobmsg_get_bool(tb[AGM_SESSION_SET_LOOPBACK_STATE]);

    AGM_LOGE("Calling agm_session_set_loopback\n");
    if (agm_session_set_loopback(capture_session_id,
                                 playback_session_id,
                                 (bool)state) != 0) {
        AGM_LOGE("agm_session_set_loopback failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_set_loopback_capture_session_id", capture_session_id);
    blobmsg_add_u32(&b, "agm_session_set_loopback_playback_session_id", playback_session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_BOOL, "agm_session_set_loopback_state", &state, sizeof(state));

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    ubus_send_reply(ctx, req, b.head);
    return 0;
}

// ------------ ipc_agm_session_set_metadata ----------------
static int ipc_agm_session_set_metadata(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_SET_METADATA_MAX];
    uint32_t size, session_id;
    uint8_t metadata = 0;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmSessionSetMetadata_policy[AGM_SESSION_SET_METADATA_SESSION_ID].name = "AgmSessionSetMetadata_session_id";
    AgmSessionSetMetadata_policy[AGM_SESSION_SET_METADATA_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionSetMetadata_policy[AGM_SESSION_SET_METADATA_SIZE].name = "AgmSessionSetMetadata_size";
    AgmSessionSetMetadata_policy[AGM_SESSION_SET_METADATA_SIZE].type = BLOBMSG_TYPE_INT8;
    AgmSessionSetMetadata_policy[AGM_SESSION_SET_METADATA_METADATA].name = "AgmSessionSetMetadata_metadata";
    AgmSessionSetMetadata_policy[AGM_SESSION_SET_METADATA_METADATA].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionSetMetadata_policy, __AGM_SESSION_SET_METADATA_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_SET_METADATA_SESSION_ID] || !tb[AGM_SESSION_SET_METADATA_SIZE] || !tb[AGM_SESSION_SET_METADATA_METADATA]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_SET_METADATA_SESSION_ID]);
    size = blobmsg_get_u32(tb[AGM_SESSION_SET_METADATA_SIZE]);
    metadata = blobmsg_get_u8(tb[AGM_SESSION_SET_METADATA_METADATA]);

    AGM_LOGE("calling agm_session_set_metadata\n");
    if (agm_session_set_metadata(session_id, size, (uint8_t *)metadata) != 0) {
        AGM_LOGE("agm_session_set_metadata failed.");
        /*free(metadata);
        metadata = NULL;*/
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_set_metadata_session_id", session_id);
    blobmsg_add_u32(&b, "agm_session_set_metadata_size", size);
    blobmsg_add_u8(&b, "agm_session_set_metadata_metadata", metadata);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ------------- ipc_agm_session_aif_set_metadata -------------------
static int ipc_agm_session_aif_set_metadata(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_AIF_SET_METADATA_MAX];
    uint32_t aif_id, size, session_id;
    uint8_t metadata;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmSessionAifSetMetadata_policy[AGM_SESSION_AIF_SET_METADATA_AIF_ID].name = "AgmSessionAifSetMetadata_aif_id";
    AgmSessionAifSetMetadata_policy[AGM_SESSION_AIF_SET_METADATA_AIF_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionAifSetMetadata_policy[AGM_SESSION_AIF_SET_METADATA_SESSION_ID].name = "AgmSessionAifSetMetadata_session_id";
    AgmSessionAifSetMetadata_policy[AGM_SESSION_AIF_SET_METADATA_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionAifSetMetadata_policy[AGM_SESSION_AIF_SET_METADATA_SIZE].name = "AgmSessionAifSetMetadata_metadata";
    AgmSessionAifSetMetadata_policy[AGM_SESSION_AIF_SET_METADATA_SIZE].type = BLOBMSG_TYPE_INT8;
    AgmSessionAifSetMetadata_policy[AGM_SESSION_AIF_SET_METADATA_SIZE].name = "AgmSessionAifSetMetadata_size";
    AgmSessionAifSetMetadata_policy[AGM_SESSION_AIF_SET_METADATA_SIZE].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionAifSetMetadata_policy, __AGM_SESSION_AIF_SET_METADATA_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_AIF_SET_METADATA_AIF_ID] || !tb[AGM_SESSION_AIF_SET_METADATA_SESSION_ID] || !tb[AGM_SESSION_AIF_SET_METADATA_SIZE] ||
                    !tb[AGM_SESSION_AIF_SET_METADATA_METADATA]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    aif_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_SET_METADATA_AIF_ID]);
    session_id = blobmsg_get_u32(tb[AGM_SESSION_AIF_SET_METADATA_SESSION_ID]);
    size = blobmsg_get_u32(tb[AGM_SESSION_AIF_SET_METADATA_SIZE]);
    metadata = blobmsg_get_u8(tb[AGM_SESSION_AIF_SET_METADATA_METADATA]);

    AGM_LOGE("Calling agm_session_aif_set_metadata\n");
    if (agm_session_aif_set_metadata(session_id,
                                     aif_id,
                                     size,
                                     (uint8_t *)metadata) != 0) {
        AGM_LOGE("agm_session_aif_set_metadata failed.");
        /*free(metadata);
        metadata = NULL;*/
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_aif_set_metadata_aif_id", aif_id);
    blobmsg_add_u32(&b, "agm_session_aif_set_metadata_session_id", session_id);
    blobmsg_add_u32(&b, "agm_session_aif_set_metadata_size", size);
    blobmsg_add_u8(&b, "agm_session_aif_set_metadata_metadata", metadata);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    ubus_send_reply(ctx, req, b.head);
    return 0;
}

// ----------- ipc_agm_audio_intf_set_metadata -------------
static int ipc_agm_audio_intf_set_metadata(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_AIF_SET_METADATA_MAX];
    uint32_t aif_id, size;
    uint8_t metadata;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmAifSetMetadata_policy[AGM_AIF_SET_METADATA_AIF_ID].name = "AgmAifSetMetadata_aif_id";
    AgmAifSetMetadata_policy[AGM_AIF_SET_METADATA_AIF_ID].type = BLOBMSG_TYPE_INT32;
    AgmAifSetMetadata_policy[AGM_AIF_SET_METADATA_METADATA].name = "AgmAifSetMetadata_metadata";
    AgmAifSetMetadata_policy[AGM_AIF_SET_METADATA_METADATA].type = BLOBMSG_TYPE_INT8;
    AgmAifSetMetadata_policy[AGM_AIF_SET_METADATA_SIZE].name = "AgmAifSetMetadata_size";
    AgmAifSetMetadata_policy[AGM_AIF_SET_METADATA_SIZE].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmAifSetMetadata_policy, __AGM_AIF_SET_METADATA_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_AIF_SET_METADATA_AIF_ID] || !tb[AGM_AIF_SET_METADATA_SIZE] || !tb[AGM_AIF_SET_METADATA_METADATA]) {
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    aif_id = blobmsg_get_u32(tb[AGM_AIF_SET_METADATA_AIF_ID]);
    size = blobmsg_get_u32(tb[AGM_AIF_SET_METADATA_SIZE]);
    metadata = blobmsg_get_u8(tb[AGM_AIF_SET_METADATA_METADATA]);

    AGM_LOGE("Calling agm_aif_set_metadata\n");
    if (agm_aif_set_metadata(aif_id, size, (uint8_t *)metadata) != 0) {
        AGM_LOGE("agm_aif_set_metadata failed.");
        /*free(metadata);
        metadata = NULL;*/
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_aif_set_metadata_aif_id", aif_id);
    blobmsg_add_u32(&b, "agm_aif_set_metadata_size", size);
    blobmsg_add_u8(&b, "agm_aif_set_metadata_metadata", metadata);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    ubus_send_reply(ctx, req, b.head);
    return 0;
}

// ------------ ipc_agm_aif_set_media_config --------------
static int ipc_agm_aif_set_media_config(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_AIF_SET_MEDIA_CONFIG_MAX];
    uint32_t aif_id;
    struct agm_media_config *media_config;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmAifSetMediaConfig_policy[AGM_AIF_SET_MEDIA_CONFIG_AIF_ID].name = "AgmAifSetMediaConfig_aif_id";
    AgmAifSetMediaConfig_policy[AGM_AIF_SET_MEDIA_CONFIG_AIF_ID].type = BLOBMSG_TYPE_INT32;
    AgmAifSetMediaConfig_policy[AGM_AIF_SET_MEDIA_CONFIG_MEDIA_CONFIG].name = "AgmAifSetMediaConfig_media_config";
    AgmAifSetMediaConfig_policy[AGM_AIF_SET_MEDIA_CONFIG_MEDIA_CONFIG].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmAifSetMediaConfig_policy, __AGM_AIF_SET_MEDIA_CONFIG_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_AIF_SET_MEDIA_CONFIG_AIF_ID] || !tb[AGM_AIF_SET_MEDIA_CONFIG_MEDIA_CONFIG]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    aif_id = blobmsg_get_u32(tb[AGM_AIF_SET_MEDIA_CONFIG_AIF_ID]);
    media_config = (struct agm_media_config*)blobmsg_data(tb[AGM_AIF_SET_MEDIA_CONFIG_MEDIA_CONFIG]);

    AGM_LOGE("Calling agm_aif_set_media_config\n");
    if (agm_aif_set_media_config(aif_id, media_config)) {
        AGM_LOGE("agm_aif_set_media_config failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_aif_set_media_config_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_aif_set_media_config_media_config", media_config, sizeof(*media_config));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ---------- ipc_agm_get_buffer_timestamp ----------------
static int ipc_agm_get_buffer_timestamp(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_GET_BUFFER_TIMESTAMP_MAX];
    uint64_t session_id;
    uint64_t timestamp = 0;

    AGM_LOGE("%s Defining the return policies\n",__func__);
    AgmGetBufferTimestamp_policy[AGM_GET_BUFFER_TIMESTAMP_SESSION_ID].name = "AgmGetBufferTimestamp_session_id";
    AgmGetBufferTimestamp_policy[AGM_GET_BUFFER_TIMESTAMP_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    /*AgmGetBufferTimestamp_policy[AGM_GET_BUFFER_TIMESTAMP_TIMESTAMP].name = "AgmGetBufferTimestamp_timestamp";
    AgmGetBufferTimestamp_policy[AGM_GET_BUFFER_TIMESTAMP_TIMESTAMP].type = BLOBMSG_TYPE_INT64;*/

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmGetBufferTimestamp_policy, __AGM_GET_BUFFER_TIMESTAMP_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_GET_BUFFER_TIMESTAMP_SESSION_ID] ) {
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_GET_BUFFER_TIMESTAMP_SESSION_ID]);
    //timestamp = blobmsg_get_u64(tb[AGM_GET_BUFFER_TIMESTAMP_TIMESTAMP]);

    AGM_LOGE("Calling agm_get_buffer_timestamp\n");
    if (agm_get_buffer_timestamp(session_id, &timestamp)) {
        AGM_LOGE("agm_get_buffer_timestamp failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_get_buffer_timestamp_session_id", session_id);
    blobmsg_add_u64(&b, "agm_get_buffer_timestamp_timestamp", timestamp);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ------------------- ipc_agm_get_hw_processed_buff_cnt ---------------
static int ipc_agm_get_hw_processed_buff_cnt(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg){
    struct blob_attr *tb[__AGM_GET_HW_PROCESSED_BUF_COUNT_MAX];
    uint64_t handle;
    enum direction *dir;
    size_t buf_count;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmGetHwProcessedBufCount_policy[AGM_GET_HW_PROCESSED_BUF_COUNT_SESSION_HANDLE].name = "AgmGetHwProcessedBufCount_session_handle" ;
    AgmGetHwProcessedBufCount_policy[AGM_GET_HW_PROCESSED_BUF_COUNT_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    AgmGetHwProcessedBufCount_policy[AGM_GET_HW_PROCESSED_BUF_COUNT_DIRECTION].name = "AgmGetHwProcessedBufCount_direction";
    AgmGetHwProcessedBufCount_policy[AGM_GET_HW_PROCESSED_BUF_COUNT_DIRECTION].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmGetHwProcessedBufCount_policy, __AGM_GET_HW_PROCESSED_BUF_COUNT_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_GET_HW_PROCESSED_BUF_COUNT_SESSION_HANDLE] || !tb[AGM_GET_HW_PROCESSED_BUF_COUNT_DIRECTION] ) {
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_GET_HW_PROCESSED_BUF_COUNT_SESSION_HANDLE]);
    dir = (enum direction*) blobmsg_data(tb[AGM_GET_HW_PROCESSED_BUF_COUNT_DIRECTION]);

    AGM_LOGE("Calling agm_get_hw_processed_buff_cnt\n");
    buf_count = agm_get_hw_processed_buff_cnt(handle,
                                              *dir);

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b,0);
    blobmsg_add_u64(&b, "agm_get_hw_processed_buff_cnt_session_handle", handle);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_get_hw_processed_buff_cnt_direction", &dir, sizeof(dir));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_get_hw_processed_buff_cnt_buffer", &buf_count, sizeof(buf_count));

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    ubus_send_reply(ctx, req, b.head);
    return 0;
}

// ------------ ipc_agm_get_session_time --------------
static int ipc_agm_get_session_time(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg) {
    struct blob_attr *tb[__AGM_SESSION_GET_TIME_MAX];
    uint64_t handle;
    uint64_t timestamp = 0;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionGetTime_policy[AGM_SESSION_GET_TIME_SESSION_HANDLE].name = "AgmSessionGetTime_session_handle";
    AgmSessionGetTime_policy[AGM_SESSION_GET_TIME_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    /*AgmSessionGetTime_policy[AGM_SESSION_GET_TIME_TIMESTAMP].name = "AgmSessionGetTime_timestamp";
    AgmSessionGetTime_policy[AGM_SESSION_GET_TIME_TIMESTAMP].type = BLOBMSG_TYPE_INT64;*/

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionGetTime_policy, __AGM_SESSION_GET_TIME_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_GET_TIME_SESSION_HANDLE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_SESSION_GET_TIME_SESSION_HANDLE]);
    //timestamp = blobmsg_get_u64(tb[AGM_SESSION_GET_TIME_TIMESTAMP]);

    AGM_LOGE("Calling agm_get_session_time\n");
    if (agm_get_session_time(handle, &timestamp)) {
        AGM_LOGE("agm_get_session_time failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_get_time_session_handle", handle);
    blobmsg_add_u64(&b, "agm_session_get_time_timestamp", timestamp);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ---------- ipc_agm_session_eos ---------
static int ipc_agm_session_eos(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg) {
    struct blob_attr *tb[__AGM_SESSION_EOS_MAX];
    uint64_t handle;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionEos_policy[AGM_SESSION_EOS_SESSION_HANDLE].name = "AgmSessionEos_session_handle";
    AgmSessionEos_policy[AGM_SESSION_EOS_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionEos_policy, __AGM_SESSION_EOS_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_EOS_SESSION_HANDLE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_SESSION_EOS_SESSION_HANDLE]);

    AGM_LOGE("Calling agm_session_eos\n");
    if (agm_session_eos(handle)) {
        AGM_LOGE("agm_session_eos failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_eos_session_handle", handle);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ---------- ipc_agm_session_set_config ----------
static int ipc_agm_session_set_config(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg){
    struct blob_attr *tb[__AGM_SESSION_SET_CONFIG_MAX];
    uint64_t handle;
    struct agm_session_config *session_config;
    struct agm_media_config *media_config;
    struct agm_buffer_config *buffer_config;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionSetConfig_policy[AGM_SESSION_SET_CONFIG_SESSION_HANDLE].name = "AgmSessionSetConfig_session_handle";
    AgmSessionSetConfig_policy[AGM_SESSION_SET_CONFIG_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    AgmSessionSetConfig_policy[AGM_SESSION_SET_CONFIG_SESSION_CONFIG].name = "AgmSessionSetConfig_session_config";
    AgmSessionSetConfig_policy[AGM_SESSION_SET_CONFIG_SESSION_CONFIG].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionSetConfig_policy[AGM_SESSION_SET_CONFIG_MEDIA_CONFIG].name = "AgmSessionSetConfig_media_config";
    AgmSessionSetConfig_policy[AGM_SESSION_SET_CONFIG_MEDIA_CONFIG].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionSetConfig_policy[AGM_SESSION_SET_CONFIG_BUFFER_CONFIG].name = "AgmSessionSetConfig_buffer_config";
    AgmSessionSetConfig_policy[AGM_SESSION_SET_CONFIG_BUFFER_CONFIG].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionSetConfig_policy, __AGM_SESSION_SET_CONFIG_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_SET_CONFIG_BUFFER_CONFIG] || !tb[AGM_SESSION_SET_CONFIG_MEDIA_CONFIG] ||
            !tb[AGM_SESSION_SET_CONFIG_SESSION_CONFIG] || !tb[AGM_SESSION_SET_CONFIG_SESSION_HANDLE] ) {
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_SESSION_SET_CONFIG_SESSION_HANDLE]);
    session_config = (struct agm_session_config*) blobmsg_data(tb[AGM_SESSION_SET_CONFIG_SESSION_CONFIG]);
    media_config = (struct agm_media_config*)blobmsg_data(tb[AGM_SESSION_SET_CONFIG_MEDIA_CONFIG]);
    buffer_config = (struct agm_buffer_config*)blobmsg_data(tb[AGM_SESSION_SET_CONFIG_BUFFER_CONFIG]);

    AGM_LOGE("Calling agm_session_set_config\n");
    if (agm_session_set_config(handle,
                               session_config,
                               media_config,
                               buffer_config)) {
        AGM_LOGE("agm_session_set_config failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_set_config_session_handle", handle);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_set_config_session_config", session_config, sizeof(*session_config));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_set_config_media_config", media_config, sizeof(*media_config));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_set_config_buffer_config", buffer_config, sizeof(*buffer_config));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

static int ipc_agm_session_write(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg){
    struct blob_attr *tb[__AGM_SESSION_WRITE_MAX];
    uint64_t handle;
    void *buff;
    size_t *byte_count;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionWrite_policy[AGM_SESSION_WRITE_SESSION_HANDLE].name = "AgmSessionWrite_session_handle";
    AgmSessionWrite_policy[AGM_SESSION_WRITE_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    AgmSessionWrite_policy[AGM_SESSION_WRITE_BUFFER].name = "AgmSessionWrite_buffer";
    AgmSessionWrite_policy[AGM_SESSION_WRITE_BUFFER].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionWrite_policy[AGM_SESSION_WRITE_BYTE_COUNT].name = "AgmSessionWrite_byte_count";
    AgmSessionWrite_policy[AGM_SESSION_WRITE_BYTE_COUNT].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionWrite_policy, __AGM_SESSION_WRITE_MAX, tb, blob_data(msg), blob_len(msg));
    if (!tb[AGM_SESSION_WRITE_SESSION_HANDLE] || !tb[AGM_SESSION_WRITE_BUFFER] || !tb[AGM_SESSION_WRITE_BYTE_COUNT]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    buff = blobmsg_data(tb[AGM_SESSION_WRITE_BUFFER]);
    handle = blobmsg_get_u64(tb[AGM_SESSION_WRITE_SESSION_HANDLE]);
    byte_count = (size_t* )blobmsg_data(tb[AGM_SESSION_WRITE_BYTE_COUNT]);

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_write_session_handle", handle);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_write_handle", buff, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_write_byte_count", byte_count, sizeof(*byte_count));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// --------- ipc_agm_session_read -----------
static int ipc_agm_session_read(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg){
    struct blob_attr *tb[__AGM_SESSION_READ_MAX];
    uint64_t handle;
    void *buff;
    size_t *byte_count;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionRead_policy[AGM_SESSION_READ_SESSION_HANDLE].name = "AgmSessionRead_session_handle";
    AgmSessionRead_policy[AGM_SESSION_READ_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    AgmSessionRead_policy[AGM_SESSION_READ_BUFFER].name = "AgmSessionRead_buffer";
    AgmSessionRead_policy[AGM_SESSION_READ_BUFFER].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionRead_policy[AGM_SESSION_READ_BYTE_COUNT].name = "AgmSessionRead_byte_count";
    AgmSessionRead_policy[AGM_SESSION_READ_BYTE_COUNT].type = BLOBMSG_TYPE_UNSPEC;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionRead_policy, __AGM_SESSION_READ_MAX, tb, blob_data(msg), blob_len(msg));
    if (!tb[AGM_SESSION_READ_SESSION_HANDLE] || !tb[AGM_SESSION_READ_BUFFER] || !tb[AGM_SESSION_READ_BYTE_COUNT]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    buff = blobmsg_data(tb[AGM_SESSION_READ_BUFFER]);
    handle = blobmsg_get_u64(tb[AGM_SESSION_READ_SESSION_HANDLE]);
    byte_count = (size_t* )blobmsg_data(tb[AGM_SESSION_READ_BYTE_COUNT]);

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_read_session_handle", handle);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_read_handle", buff, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_read_byte_count", byte_count, sizeof(*byte_count));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

//--------- ipc_agm_session_pause ----------
static int ipc_agm_session_pause(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg) {
    struct blob_attr *tb[__AGM_SESSION_PAUSE_MAX];
    uint64_t handle;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionPause_policy[AGM_SESSION_PAUSE_SESSION_HANDLE].name = "AgmSessionPause_session_handle";
    AgmSessionPause_policy[AGM_SESSION_PAUSE_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionPause_policy, __AGM_SESSION_PAUSE_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_PAUSE_SESSION_HANDLE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_SESSION_PAUSE_SESSION_HANDLE]);

    AGM_LOGE("Calling agm_session_pause");
    if (agm_session_pause(handle)) {
        AGM_LOGE("agm_session_pause failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_pause_session_handle", handle);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}
// --------- ipc_agm_session_resume -----------
static int ipc_agm_session_resume(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg) {
    struct blob_attr *tb[__AGM_SESSION_RESUME_MAX];
    uint64_t handle;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionResume_policy[AGM_SESSION_RESUME_SESSION_HANDLE].name = "AgmSessionResume_session_handle";
    AgmSessionResume_policy[AGM_SESSION_RESUME_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionResume_policy, __AGM_SESSION_RESUME_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_RESUME_SESSION_HANDLE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_SESSION_RESUME_SESSION_HANDLE]);

    AGM_LOGE("calling agm_session_resume\n");
    if (agm_session_resume(handle)) {
        AGM_LOGE("agm_session_resume failed.");
        return UBUS_STATUS_UNKNOWN_ERROR;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_resume_session_handle", handle);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    ubus_send_reply(ctx, req, b.head);
    return 0;
}

// ------ ipc_agm_session_prepare -------
static int ipc_agm_session_prepare(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg) {
    struct blob_attr *tb[__AGM_SESSION_PREPARE_MAX];
    uint64_t handle;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionPrepare_policy[AGM_SESSION_PREPARE_SESSION_HANDLE].name = "AgmSessionPrepare_session_handle";
    AgmSessionPrepare_policy[AGM_SESSION_PREPARE_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionPrepare_policy, __AGM_SESSION_PREPARE_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_PREPARE_SESSION_HANDLE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_SESSION_PREPARE_SESSION_HANDLE]);

    AGM_LOGE("Calling agm_session_prepare\n");
    if (agm_session_prepare(handle)) {
        AGM_LOGE("agm_session_prepare failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_prepare_session_handle", handle);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// -------- ipc_agm_session_start ------------
static int ipc_agm_session_start(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg) {
    struct blob_attr *tb[__AGM_SESSION_START_MAX];
    uint64_t handle;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionStart_policy[AGM_SESSION_START_SESSION_HANDLE].name = "AgmSessionStart_session_handle";
    AgmSessionStart_policy[AGM_SESSION_START_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionStart_policy, __AGM_SESSION_START_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_START_SESSION_HANDLE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_SESSION_START_SESSION_HANDLE]);

    AGM_LOGE("Calling agm_session_start\n");
    if (agm_session_start(handle)) {
        AGM_LOGE("agm_session_start failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_start_session_handle", handle);
    ubus_send_reply(ctx, req, b.head);
    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// -------- ipc_agm_session_stop ------------
static int ipc_agm_session_stop(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg) {
    struct blob_attr *tb[__AGM_SESSION_STOP_MAX];
    uint64_t handle;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionStop_policy[AGM_SESSION_STOP_SESSION_HANDLE].name = "AgmSessionStop_session_handle";
    AgmSessionStop_policy[AGM_SESSION_STOP_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionStop_policy, __AGM_SESSION_STOP_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_STOP_SESSION_HANDLE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_SESSION_STOP_SESSION_HANDLE]);

    AGM_LOGE("Calling agm_session_stop\n");
    if (agm_session_stop(handle)) {
        AGM_LOGE("%s :agm_session_stop failed.", __func__);
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_stop_session_handle", handle);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ------ IPC_AGM_SESSION_CLOSE --------
static int ipc_agm_session_close(struct ubus_context *ctx, struct ubus_object* obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr* msg) {
    struct blob_attr *tb[__AGM_SESSION_CLOSE_MAX];
    uint64_t handle;
    agm_client_session_data *ses_data = NULL;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionClose_policy[AGM_SESSION_CLOSE_SESSION_HANDLE].name = "AgmSessionClose_session_handle";
    AgmSessionClose_policy[AGM_SESSION_CLOSE_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionClose_policy, __AGM_SESSION_CLOSE_MAX, tb, blob_data(msg), blob_len(msg));
    if(!tb[AGM_SESSION_CLOSE_SESSION_HANDLE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    handle = blobmsg_get_u64(tb[AGM_SESSION_CLOSE_SESSION_HANDLE]);
    ses_data = (agm_client_session_data *) handle;

    AGM_LOGE("Hash Table Remove\n");
    g_hash_table_remove(ses_hash_table, GINT_TO_POINTER(ses_data->session_id));

    AGM_LOGE("Calling agm_session_close\n");
    if (agm_session_close(handle)) {
        AGM_LOGE("agm_session_close failed.");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_CONNECTION_FAILED;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "agm_session_close_session_handle", handle);
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

// ------ IPC_AGM_SESSION_OPEN -------
static int ipc_agm_session_open(struct ubus_context *ctx, struct ubus_object *obj,
              struct ubus_request_data *req, const char *method,
              struct blob_attr *msg){
    struct blob_attr *tb[__AGM_SESSION_OPEN_MAX];
    uint32_t session_id;
    uint64_t handle;
    enum agm_session_mode *sess_mode;
    agm_client_session_data *ses_data = NULL;

    AGM_LOGE("%s Defining the Return policies\n",__func__);
    AgmSessionOpen_policy[AGM_SESSION_OPEN_SESSION_ID].name = "AgmSessionOpen_session_id";
    AgmSessionOpen_policy[AGM_SESSION_OPEN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    AgmSessionOpen_policy[AGM_SESSION_OPEN_SESSION_MODE].name = "AgmSessionOpen_session_mode";
    AgmSessionOpen_policy[AGM_SESSION_OPEN_SESSION_MODE].type = BLOBMSG_TYPE_UNSPEC;
    AgmSessionOpen_policy[AGM_SESSION_OPEN_SESSION_HANDLE].name = "AgmSessionOpen_session_handle";
    AgmSessionOpen_policy[AGM_SESSION_OPEN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;

    AGM_LOGE("%s Getting data from Client\n",__func__);
    blobmsg_parse(AgmSessionOpen_policy, __AGM_SESSION_OPEN_MAX, tb, blob_data(msg), blob_len(msg));
    if (!tb[AGM_SESSION_OPEN_SESSION_ID] || !tb[AGM_SESSION_OPEN_SESSION_HANDLE] || !tb[AGM_SESSION_OPEN_SESSION_MODE]){
        AGM_LOGE("%s Error in Getting data from Client\n",__func__);
        return UBUS_STATUS_INVALID_ARGUMENT;
    }

    session_id = blobmsg_get_u32(tb[AGM_SESSION_OPEN_SESSION_ID]);
    handle = blobmsg_get_u64(tb[AGM_SESSION_OPEN_SESSION_HANDLE]);
    sess_mode = (enum agm_session_mode* )blobmsg_data(tb[AGM_SESSION_OPEN_SESSION_MODE]);

    AGM_LOGE("Hash Table\n");
    if((agm_client_session_data*)g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id)) == NULL ) {
        ses_data = (agm_client_session_data*)g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id));
        ses_data = (agm_client_session_data *)g_malloc0(sizeof(agm_client_session_data));
        ses_data -> session_id = session_id;
        ses_data -> ctx = ctx;
        g_hash_table_insert(ses_hash_table, GINT_TO_POINTER(session_id), ses_data);
    }

    AGM_LOGE("Calling agm_session_open\n");
    if (agm_session_open(session_id, *sess_mode, &handle)) {
        //agm_free_session(ses_data);
        AGM_LOGE("agm_session_open falied\n");
        agm_ubus_send_error(req, UBUS_STATUS_CONNECTION_FAILED);
        return UBUS_STATUS_UNKNOWN_ERROR;
    }

    AGM_LOGE("%s Replying to Client\n",__func__);
    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "agm_session_open_session_id", session_id);
    blobmsg_add_u64(&b, "agm_session_open_session_handle", handle);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "agm_session_open_session_mode", &sess_mode, sizeof(sess_mode));
    ubus_send_reply(ctx, req, b.head);

    AGM_LOGE("%s Reply sent to Client\n",__func__);
    return 0;
}

int ipc_agm_init() {
    int ret = 0;
    int rc = 0;
    const char *ubus_socket = UBUS_UNIX_SOCKET;
    /*ret = agm_ubus_new_connection(ubus_socket);
    if (ret){
        fprintf(stderr, "Failed to create a UBUS connection\n");
        rc = -1;
        goto exit;
    }*/
    ctx = agm_ubus_new_connection(ubus_socket);
    ret = ubus_add_object(ctx, &agm_ubus_module_object);
    if (ret){
        AGM_LOGE("Failed to add agm_ubus_module_object: %s\n", ubus_strerror(ret));
        rc = -1;
        goto exit;
    }
    ret = ubus_add_object(ctx, &agm_ubus_session_object);
    if (ret){
        AGM_LOGE("Failed to add agm_ubus_session_object: %s\n", ubus_strerror(ret));
        rc = -1;
        goto exit;
    }
    ret = ubus_register_subscriber(ctx, &agm_ubus_event);
    if (ret){
        AGM_LOGE("Failed to add watch handler: %s\n", ubus_strerror(ret));
        rc = -1;
        goto exit;
    }
    uloop_run();
exit:
    return rc;
}

void ipc_agm_deinit() {
    ubus_free(ctx);
    uloop_done();
    g_hash_table_remove_all(ses_hash_table);
    g_hash_table_unref(ses_hash_table);
}
