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
#include <utils.h>
#include <stdlib.h>
#define UBUS_UNIX_SOCKET "/var/run/ubus/ubus.sock"

static struct ubus_context *ctx;
static struct blob_buf b;
GHashTable *ses_hash_table;

typedef struct {
    struct ubus_context* ctx;
    uint32_t session_id;
}agm_client_session_data;

enum {
    RETURN_SESSION_ID,
    RETURN_SESSION_HANDLE,
    RETURN_SESSION_MODE,
    RETURN_BUFFER,
    RETURN_BYTE_COUNT,
    RETURN_TIMESTAMP,
    RETURN_MEDIA_CONFIG,
    RETURN_SESSION_CONFIG,
    RETURN_BUFFER_CONFIG,
    RETURN_DIRECTION,
    RETURN_AIF_ID,
    RETURN_METADATA,
    RETURN_SIZE,
    RETURN_CAPTURE_SESSION_ID,
    RETURN_PLAYBACK_SESSION_ID,
    RETURN_STATE,
    RETURN_TAG_CONFIG,
    RETURN_AIF_LIST,
    RETURN_AIF_INFO,
    RETURN_PAYLOAD,
    RETURN_REG_CFG,
    RETURN_CLIENT_DATA,
    RETURN_EVENT_TYPE,
    RETURN_AGM_EVENT_CB,
    RETURN_CAL_CONFIG,
    RETURN_FLAG,
    RETURN_BUF_INFO,
    RETURN_ERROR,
    __RETURN_MAX,
};

static struct blobmsg_policy agm_session_get_buf_info_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_get_params_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_aif_set_cal_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_aif_set_params_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_aif_set_params_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_aif_get_tag_module_info_size_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_aif_get_tag_module_info_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_aif_connect_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_set_ec_ref_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_deregister_cb_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_register_cb_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_register_for_events_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_set_params_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_get_aif_info_list_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_get_aif_info_list_size_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_set_params_with_tag_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_set_loopback_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_set_metadata_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_aif_set_metadata_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_aif_set_metadata_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_aif_set_media_config_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_get_buffer_timestamp_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_get_hw_processed_buff_cnt_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_get_session_time_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_eos_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_set_config_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_write_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_read_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_pause_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_resume_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_prepare_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_start_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_stop_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_close_return_policy[__RETURN_MAX];
static struct blobmsg_policy agm_session_open_return_policy[__RETURN_MAX];

static void agm_ubus_subscribe_cb(struct ubus_context *ctx, struct ubus_object *obj)
{
    AGM_LOGE("Subscribers active: %d\n", obj->has_subscribers);
}

static struct ubus_object agm_ubus_client_object = {
    .subscribe_cb = agm_ubus_subscribe_cb,
};

int initialize_module_data() {
    int rc = 0;
    uloop_init_t uloop_init_ptr = NULL;
    uloop_fd_add_t uloop_fd_add_ptr = NULL;
    const char *ubus_socket = UBUS_UNIX_SOCKET;
    void * uboxlibHdl = NULL;

    if (!ctx) {
        uloop_init_ptr();
        ctx = ubus_connect(ubus_socket);
        if (!ctx) {
            AGM_LOGE("Failed to connect to ubus\n");
            rc = -1;
            goto exit;
        }
        AGM_LOGE("connect to ubus success\n");
        uloop_fd_add_ptr = (uloop_fd_add_t)dlsym(uboxlibHdl, "uloop_fd_add");
        if (uloop_fd_add_ptr == NULL) {
            AGM_LOGE("dlsym error %s for uloop_init\n", dlerror());
        }
        uloop_fd_add_ptr(&ctx->sock, ULOOP_BLOCKING | ULOOP_READ);
        //ubus_add_uloop(ctx);
        ses_hash_table = g_hash_table_new(g_direct_hash, g_direct_equal);
    }
exit:
    return rc;
}

void agm_session_get_buf_info_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id, flag;
    struct agm_buf_info *buf_info;

    agm_session_get_buf_info_return_policy[RETURN_SESSION_ID].name = "agm_session_get_buf_info_session_id";
    agm_session_get_buf_info_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_get_buf_info_return_policy[RETURN_BUF_INFO].name = "agm_session_get_buf_info_buf_info";
    agm_session_get_buf_info_return_policy[RETURN_BUF_INFO].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_get_buf_info_return_policy[RETURN_FLAG].name = "agm_session_get_buf_info_flag";
    agm_session_get_buf_info_return_policy[RETURN_FLAG].type = BLOBMSG_TYPE_INT32;
    agm_session_get_buf_info_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_get_buf_info_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving Message from Server", __func__);

    blobmsg_parse(agm_session_get_buf_info_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_BUF_INFO] || !tb[RETURN_FLAG]) {
        AGM_LOGE("%s Error in the data recieved", __func__);
        return;
    }

    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    flag = blobmsg_get_u32(tb[RETURN_FLAG]);
    buf_info = (struct agm_buf_info* )blobmsg_data(tb[RETURN_BUF_INFO]);
    AGM_LOGE("%s Recieved the data from server\n", __func__);
}

int agm_session_get_buf_info(uint32_t session_id, struct agm_buf_info *buf_info, uint32_t flag) {
    uint32_t id;
    AGM_LOGD("%s\n",__func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionGetBufInfo_session_id", session_id);
    blobmsg_add_u32(&b, "AgmSessionGetBufInfo_flag", flag);

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("Failed to look up test object\n");
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionGetBufInfo", b.head, agm_session_get_buf_info_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server\n", __func__);
        return -1;
    }

    AGM_LOGE("%s: Called the server\n",__func__);
    return 0;
}

// ------------- agm_session_get_params ------------
void agm_session_get_params_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id;
    void* payload;
    size_t* size;

    agm_session_get_params_return_policy[RETURN_SESSION_ID].name = "agm_session_get_params_session_id";
    agm_session_get_params_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_get_params_return_policy[RETURN_PAYLOAD].name = "agm_session_get_params_payload";
    agm_session_get_params_return_policy[RETURN_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_get_params_return_policy[RETURN_SIZE].name = "agm_session_get_params_size";
    agm_session_get_params_return_policy[RETURN_SIZE].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_get_params_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_get_params_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving Message from Server", __func__);
    blobmsg_parse(agm_session_get_params_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_PAYLOAD] || !tb[RETURN_SIZE]) {
        AGM_LOGE("%s Error in data recieved\n", __func__);
        return;
    }

    payload = blobmsg_data(tb[RETURN_PAYLOAD]);
    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    size = (size_t* )blobmsg_data(tb[RETURN_SIZE]);
    AGM_LOGE("%s Recieved Message from Server", __func__);
}

int agm_session_get_params(uint32_t session_id, void* payload, size_t size){
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionGetParams_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionGetParams_payload", payload, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionGetParams_size", &size, sizeof(size));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s: Calling the server\n",__func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionGetParams", b.head, agm_session_get_params_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s: Error in calling Server\n", __func__);
        return -1;
    }
    AGM_LOGE("%s: Called the server\n",__func__);
    return 0;
}

// ----------  agm_session_aif_set_cal ----------------
void agm_session_aif_set_cal_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id, aif_id;
    struct agm_cal_config *cal_config;

    agm_session_aif_set_cal_return_policy[RETURN_SESSION_ID].name = "agm_session_aif_set_cal_session_id";
    agm_session_aif_set_cal_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_set_cal_return_policy[RETURN_AIF_ID].name = "agm_session_aif_set_cal_aif_id";
    agm_session_aif_set_cal_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_set_cal_return_policy[RETURN_CAL_CONFIG].name = "agm_session_aif_set_cal_cal_config";
    agm_session_aif_set_cal_return_policy[RETURN_CAL_CONFIG].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_aif_set_cal_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_aif_set_cal_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s: Recieving the data from server\n", __func__);
    blobmsg_parse(agm_session_aif_set_cal_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_AIF_ID] || !tb[RETURN_CAL_CONFIG]) {
        AGM_LOGE("%s: No return code received from server\n", __func__);
        return;
    }

    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    cal_config = (struct agm_cal_config*)blobmsg_data(tb[RETURN_CAL_CONFIG]);
    AGM_LOGE("%s: Recieved data from server\n",  __func__);
}

int agm_session_aif_set_cal(uint32_t session_id, uint32_t aif_id, struct agm_cal_config *cal_config) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionAifSetCal_session_id", session_id);
    blobmsg_add_u32(&b, "AgmSessionAifSetCal_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionAifSetCal_cal_config", cal_config, 0);

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s: Calling the server\n",__func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionAifSetCal", b.head, agm_session_aif_set_cal_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s: Called the server\n",__func__);
    return 0;
}

// -------------- agm_aif_set_params -----------------

void agm_aif_set_params_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t aif_id;
    void *payload;
    size_t *size;

    agm_aif_set_params_return_policy[RETURN_AIF_ID].name = "agm_aif_set_params_aif_id";
    agm_aif_set_params_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_aif_set_params_return_policy[RETURN_PAYLOAD].name = "agm_aif_set_params_payload";
    agm_aif_set_params_return_policy[RETURN_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;
    agm_aif_set_params_return_policy[RETURN_SIZE].name = "agm_aif_set_params_size";
    agm_aif_set_params_return_policy[RETURN_SIZE].type = BLOBMSG_TYPE_UNSPEC;
    agm_aif_set_params_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_aif_set_params_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving the data from server\n",__func__);
    blobmsg_parse(agm_aif_set_params_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_AIF_ID] || !tb[RETURN_PAYLOAD] || !tb[RETURN_SIZE]) {
        AGM_LOGE("%s No return code received from server\n",__func__);
        return;
    }

    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    payload = blobmsg_data(tb[RETURN_PAYLOAD]);
    size = (size_t*)blobmsg_data(tb[RETURN_SIZE]);
    AGM_LOGE("%s Recieved the data from server\n",__func__);
}

int agm_aif_set_params(uint32_t aif_id, void* payload, size_t size) {
    uint32_t id;
    AGM_LOGE("%s\n",__func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmAifSetParams_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmAifSetParams_payload", payload, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmAifSetParams_size", &size, sizeof(size));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n",__func__);
        return -1;
    }

    AGM_LOGE("%s: Calling the server\n",__func__);
    int ret = ubus_invoke(ctx, id, "AgmAifSetParams", b.head, agm_aif_set_params_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n",__func__);
        return -1;
    }
    AGM_LOGE("%s: Called the server\n",__func__);
    return 0;
}

// ------------- agm_session_aif_set_params -------------------

void agm_session_aif_set_params_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id, aif_id;
    void *payload;
    size_t *size;

    agm_session_aif_set_params_return_policy[RETURN_SESSION_ID].name = "agm_session_aif_set_params_session_id";
    agm_session_aif_set_params_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_set_params_return_policy[RETURN_AIF_ID].name = "agm_session_aif_set_params_aif_id";
    agm_session_aif_set_params_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_set_params_return_policy[RETURN_PAYLOAD].name = "agm_session_aif_set_params_payload";
    agm_session_aif_set_params_return_policy[RETURN_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_aif_set_params_return_policy[RETURN_SIZE].name = "agm_session_aif_set_params_size";
    agm_session_aif_set_params_return_policy[RETURN_SIZE].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_aif_set_params_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_aif_set_params_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving the data from server\n", __func__);
    blobmsg_parse(agm_session_aif_set_params_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));
    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_AIF_ID] || !tb[RETURN_PAYLOAD] || !tb[RETURN_SIZE]) {
        AGM_LOGE("%s Error in data received\n", __func__);
        return;
    }

    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    payload = blobmsg_data(tb[RETURN_PAYLOAD]);
    size = (size_t*)blobmsg_data(tb[RETURN_SIZE]);
    AGM_LOGE("%s Recieved the data from server\n", __func__);
}

int agm_session_aif_set_params(uint32_t session_id, uint32_t aif_id,
                               void* payload, size_t size) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionAifSetParams_session_id", session_id);
    blobmsg_add_u32(&b, "AgmSessionAifSetParams_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionAifSetParams_payload", payload, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionAifSetParams_size", &size, sizeof(size));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n",__func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionAifSetParams", b.head, agm_session_aif_set_params_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// --------- agm_session_aif_get_tag_module_info_size ------------

void agm_session_aif_get_tag_module_info_size_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id, aif_id;
    size_t *size;

    agm_session_aif_get_tag_module_info_size_return_policy[RETURN_SESSION_ID].name =  "agm_session_aif_get_tag_module_info_size_session_id";
    agm_session_aif_get_tag_module_info_size_return_policy[RETURN_SESSION_ID].type =  BLOBMSG_TYPE_INT32;
    agm_session_aif_get_tag_module_info_size_return_policy[RETURN_AIF_ID].name =  "agm_session_aif_get_tag_module_info_size_aif_id";
    agm_session_aif_get_tag_module_info_size_return_policy[RETURN_AIF_ID].type =  BLOBMSG_TYPE_INT32;
    agm_session_aif_get_tag_module_info_size_return_policy[RETURN_SIZE].name =  "agm_session_aif_get_tag_module_info_size_size";
    agm_session_aif_get_tag_module_info_size_return_policy[RETURN_SIZE].type =  BLOBMSG_TYPE_UNSPEC;
    agm_session_aif_get_tag_module_info_size_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_aif_get_tag_module_info_size_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving the data from server\n", __func__);
    blobmsg_parse(agm_session_aif_get_tag_module_info_size_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_AIF_ID] || !tb[RETURN_SIZE]) {
        AGM_LOGE("%s Error in the data received\n", __func__);
        return;
    }

    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    size = (size_t*)blobmsg_data(tb[RETURN_SIZE]);

    AGM_LOGE("%s Recieved the data from server\n", __func__);
}

int agm_session_aif_get_tag_module_info_size(uint32_t session_id,
                                             uint32_t aif_id,
                                             size_t *size){
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionAifGetTagModuleInfoSize_session_id", session_id);
    blobmsg_add_u32(&b, "AgmSessionAifGetTagModuleInfoSize_aif_id", aif_id);

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionAifGetTagModuleInfoSize", b.head, agm_session_aif_get_tag_module_info_size_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in Calling Server function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// -------------- agm_session_aif_get_tag_module_info -----------

void agm_session_aif_get_tag_module_info_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id, aif_id;
    void *payload;
    size_t *size;

    agm_session_aif_get_tag_module_info_return_policy[RETURN_SESSION_ID].name = "agm_session_aif_get_tag_module_info_session_id";
    agm_session_aif_get_tag_module_info_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_get_tag_module_info_return_policy[RETURN_AIF_ID].name = "agm_session_aif_get_tag_module_info_aif_id";
    agm_session_aif_get_tag_module_info_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_get_tag_module_info_return_policy[RETURN_PAYLOAD].name = "agm_session_aif_get_tag_module_info_payload";
    agm_session_aif_get_tag_module_info_return_policy[RETURN_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_aif_get_tag_module_info_return_policy[RETURN_SIZE].name = "agm_session_aif_get_tag_module_info_size";
    agm_session_aif_get_tag_module_info_return_policy[RETURN_SIZE].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_aif_get_tag_module_info_return_policy[RETURN_SIZE].name =  "agm_session_aif_get_tag_module_info_size_size";
    agm_session_aif_get_tag_module_info_return_policy[RETURN_SIZE].type =  BLOBMSG_TYPE_UNSPEC;
    agm_session_aif_get_tag_module_info_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_aif_get_tag_module_info_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving the data from server\n", __func__);
    blobmsg_parse(agm_session_aif_get_tag_module_info_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_AIF_ID] || !tb[RETURN_PAYLOAD] || !tb[RETURN_SIZE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    payload = blobmsg_data(tb[RETURN_PAYLOAD]);
    size = (size_t*)blobmsg_data(tb[RETURN_SIZE]);
    AGM_LOGE("%s Recieved the data from server\n", __func__);
}

int agm_session_aif_get_tag_module_info(uint32_t session_id,
                                        uint32_t aif_id,
                                        void *payload, size_t *size) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionAifGetTagModuleInfo_session_id", session_id);
    blobmsg_add_u32(&b, "AgmSessionAifGetTagModuleInfo_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionAifGetTagModuleInfo_size", size, sizeof(*size));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n",__func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionAifGetTagModuleInfo", b.head, agm_session_aif_get_tag_module_info_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n",__func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);

    return 0;
}

// -------- agm_session_aif_connect --------------

void agm_session_aif_connect_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id, aif_id;
    bool state;

    agm_session_aif_connect_return_policy[RETURN_SESSION_ID].name = "agm_session_aif_connect_session_id";
    agm_session_aif_connect_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_connect_return_policy[RETURN_AIF_ID].name = "agm_session_aif_connect_aif_id";
    agm_session_aif_connect_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_connect_return_policy[RETURN_STATE].name = "agm_session_aif_connect_state";
    agm_session_aif_connect_return_policy[RETURN_STATE].type = BLOBMSG_TYPE_BOOL;
    agm_session_aif_connect_return_policy[RETURN_SIZE].name =  "agm_session_aif_get_tag_module_info_size_size";
    agm_session_aif_connect_return_policy[RETURN_SIZE].type =  BLOBMSG_TYPE_UNSPEC;
    agm_session_aif_connect_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_aif_connect_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving the data from server\n", __func__);
    blobmsg_parse(agm_session_aif_connect_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_AIF_ID] || !tb[RETURN_STATE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    state = blobmsg_get_bool(tb[RETURN_STATE]);
    AGM_LOGE("%s Recieved the data from server\n", __func__);
}

int agm_session_aif_connect(uint32_t session_id,
                            uint32_t aif_id,
                            bool state) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionAifConnect_session_id", session_id);
    blobmsg_add_u32(&b, "AgmSessionAifConnect_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_BOOL, "AgmSessionAifConnect_state", &state, sizeof(state));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n",__func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionAifConnect", b.head, agm_session_aif_connect_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// --------- agm_session_set_ec_ref -----------

void agm_session_set_ec_ref_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t capture_session_id, aif_id;
    bool state;

    agm_session_set_ec_ref_return_policy[RETURN_CAPTURE_SESSION_ID].name = "agm_session_set_ec_ref_capture_session_id";
    agm_session_set_ec_ref_return_policy[RETURN_CAPTURE_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_set_ec_ref_return_policy[RETURN_AIF_ID].name = "agm_session_set_ec_ref_aif_id";
    agm_session_set_ec_ref_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_set_ec_ref_return_policy[RETURN_STATE].name = "agm_session_set_ec_ref_state";
    agm_session_set_ec_ref_return_policy[RETURN_STATE].type = BLOBMSG_TYPE_BOOL;
    agm_session_set_ec_ref_return_policy[RETURN_SIZE].name =  "agm_session_aif_get_tag_module_info_size_size";
    agm_session_set_ec_ref_return_policy[RETURN_SIZE].type =  BLOBMSG_TYPE_UNSPEC;
    agm_session_set_ec_ref_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_set_ec_ref_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving the data from server\n", __func__);
    blobmsg_parse(agm_session_set_ec_ref_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_CAPTURE_SESSION_ID] || !tb[RETURN_AIF_ID] || !tb[RETURN_STATE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    capture_session_id = blobmsg_get_u32(tb[RETURN_CAPTURE_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    state = blobmsg_get_bool(tb[RETURN_STATE]);
    AGM_LOGE("%s Recieved the data from server\n", __func__);
}

int agm_session_set_ec_ref(uint32_t capture_session_id,
                           uint32_t aif_id,
                           bool state) {
    uint32_t id;
    AGM_LOGE("%s\n",__func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionSetEcRef_capture_session_id", capture_session_id);
    blobmsg_add_u32(&b, "AgmSessionSetEcRef_aif_id", aif_id);
    blobmsg_add_field(&b,BLOBMSG_TYPE_BOOL, "AgmSessionSetEcRef_state", &state, sizeof(state));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionSetEcRef", b.head, agm_session_set_ec_ref_cb, 0, 5000);
    if(ret) {
        AGM_LOGD("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// ------------ agm_session_deregister_cb -----------------

void agm_session_deregister_cb_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id;
    enum event_type* evt_type;
    void *client_data;

    agm_session_deregister_cb_return_policy[RETURN_SESSION_ID].name = "agm_session_deregister_cb_session_id";
    agm_session_deregister_cb_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_deregister_cb_return_policy[RETURN_EVENT_TYPE].name = "agm_session_deregister_cb_event_type";
    agm_session_deregister_cb_return_policy[RETURN_EVENT_TYPE].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_deregister_cb_return_policy[RETURN_CLIENT_DATA].name = "agm_session_deregister_cb_client_data";
    agm_session_deregister_cb_return_policy[RETURN_CLIENT_DATA].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_deregister_cb_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_deregister_cb_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving the data from server\n", __func__);
    blobmsg_parse(agm_session_deregister_cb_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_EVENT_TYPE] || !tb[RETURN_CLIENT_DATA]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    evt_type = (enum event_type*)blobmsg_data(tb[RETURN_EVENT_TYPE]);
    client_data = blobmsg_data(tb[RETURN_CLIENT_DATA]);
    AGM_LOGE("%s Recieved the data from server\n", __func__);
}

static int agm_session_deregister_cb(uint32_t session_id,
                                     enum event_type evt_type,
                                     void *client_data){
    uint32_t id;
    agm_client_session_data *ses_data;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionDeregisterCb_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionDeregisterCb_event_type", &evt_type, sizeof(evt_type));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionDeregisterCb_client_data", client_data, 0);

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n",__func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionDeregisterCb", b.head, agm_session_deregister_cb_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n",__func__);
        return -1;
    }

    AGM_LOGE("%s Hash Table\n", __func__);
    if(g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id)) == NULL ) {
        ses_data = (agm_client_session_data*)g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id));
        ses_data = (agm_client_session_data *)g_malloc0(sizeof(agm_client_session_data));
        ses_data -> session_id = session_id;
        ses_data -> ctx = ctx;
        g_hash_table_insert(ses_hash_table, GINT_TO_POINTER(session_id), ses_data);
    }

    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// ------------- agm_session_register_cb ------------------

void agm_session_register_cb_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id;
    agm_event_cb* cb;
    enum event_type* evt_type;
    void *client_data;

    agm_session_register_cb_return_policy[RETURN_SESSION_ID].name = "agm_session_register_cb_session_id";
    agm_session_register_cb_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_register_cb_return_policy[RETURN_AGM_EVENT_CB].name = "agm_session_register_cb_agm_event_cb";
    agm_session_register_cb_return_policy[RETURN_AGM_EVENT_CB].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_register_cb_return_policy[RETURN_EVENT_TYPE].name = "agm_session_register_cb_event_type";
    agm_session_register_cb_return_policy[RETURN_EVENT_TYPE].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_register_cb_return_policy[RETURN_CLIENT_DATA].name = "agm_session_register_cb_client_data";
    agm_session_register_cb_return_policy[RETURN_CLIENT_DATA].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_register_cb_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_register_cb_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving the data from server\n", __func__);
    blobmsg_parse(agm_session_register_cb_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_AGM_EVENT_CB] || !tb[RETURN_EVENT_TYPE] || !tb[RETURN_CLIENT_DATA]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    cb = (agm_event_cb* )blobmsg_data(tb[RETURN_AGM_EVENT_CB]);
    evt_type = (enum event_type*)blobmsg_data(tb[RETURN_EVENT_TYPE]);
    client_data = blobmsg_data(tb[RETURN_CLIENT_DATA]);
    AGM_LOGE("%s Recieving the data from server\n", __func__);
}

int agm_session_register_cb(uint32_t session_id,
                            agm_event_cb cb,
                            enum event_type evt_type,
                            void *client_data) {
    uint32_t id;
    agm_client_session_data *ses_data = NULL;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionRegisterCb_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionRegisterCb_agm_event_cb", &cb, sizeof(cb));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionRegisterCb_event_type", &evt_type, sizeof(evt_type));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionRegisterCb_client_data", client_data, 0);

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n",__func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionRegisterCb", b.head, agm_session_register_cb_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Hash Table\n", __func__);
    if(g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id)) == NULL ) {
        ses_data = (agm_client_session_data*)g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id));
        ses_data = (agm_client_session_data *)g_malloc0(sizeof(agm_client_session_data));
        ses_data -> session_id = session_id;
        ses_data -> ctx = ctx;
        g_hash_table_insert(ses_hash_table, GINT_TO_POINTER(session_id), ses_data);
    }

    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// -------------- agm_session_register_for_events -----------

void agm_session_register_for_events_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id;
    struct agm_event_reg_cfg *evt_reg_cfg;

    agm_session_register_for_events_return_policy[RETURN_SESSION_ID].name = "agm_session_register_for_events_session_id";
    agm_session_register_for_events_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_register_for_events_return_policy[RETURN_REG_CFG].name = "agm_session_register_for_events_reg_cfg";
    agm_session_register_for_events_return_policy[RETURN_REG_CFG].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_register_for_events_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_register_for_events_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving the data from server\n", __func__);
    blobmsg_parse(agm_session_register_for_events_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_REG_CFG] ) {
        AGM_LOGE("%s No return code received from server\n",__func__);
        return;
    }
    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    evt_reg_cfg = (struct agm_event_reg_cfg* )blobmsg_data(tb[RETURN_REG_CFG]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_register_for_events(uint32_t session_id,
                                                struct agm_event_reg_cfg *evt_reg_cfg) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionRegisterForEvents_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionRegisterForEvents_reg_cfg", evt_reg_cfg, 0);

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionRegisterForEvents", b.head, agm_session_register_for_events_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// --------- agm_session_set_params --------------

void agm_session_set_params_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id;
    void* payload;
    size_t* size;

    agm_session_set_params_return_policy[RETURN_SESSION_ID].name = "agm_session_set_params_session_id";
    agm_session_set_params_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_set_params_return_policy[RETURN_PAYLOAD].name = "agm_session_set_params_payload";
    agm_session_set_params_return_policy[RETURN_PAYLOAD].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_set_params_return_policy[RETURN_SIZE].name = "agm_session_set_params_size";
    agm_session_set_params_return_policy[RETURN_SIZE].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_set_params_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_set_params_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_set_params_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_PAYLOAD] || !tb[RETURN_SIZE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    payload = blobmsg_data(tb[RETURN_PAYLOAD]);
    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    size = (size_t* )blobmsg_data(tb[RETURN_SIZE]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_set_params(uint32_t session_id, void* payload, size_t size){
    uint32_t id;
    blob_buf_init(&b, 0);
    AGM_LOGE("%s\n", __func__);

    blobmsg_add_u32(&b, "AgmSessionSetParams_session_id", session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionSetParams_payload", payload, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionSetParams_size", &size, sizeof(size));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionSetParams", b.head, agm_session_set_params_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// -------------- agm_get_aif_info_list ---------------

void agm_get_aif_info_list_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    size_t *num_aif_info;
    struct aif_info *aif_list;

    agm_get_aif_info_list_return_policy[RETURN_AIF_INFO].name = "agm_get_aif_info_list_aif_info";
    agm_get_aif_info_list_return_policy[RETURN_AIF_INFO].type = BLOBMSG_TYPE_UNSPEC;
    agm_get_aif_info_list_return_policy[RETURN_AIF_LIST].name = "agm_get_aif_info_list_aif_list";
    agm_get_aif_info_list_return_policy[RETURN_AIF_LIST].type = BLOBMSG_TYPE_UNSPEC;
    agm_get_aif_info_list_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_get_aif_info_list_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_get_aif_info_list_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_AIF_INFO] || !tb[RETURN_AIF_LIST]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    num_aif_info = (size_t*)blobmsg_data(tb[RETURN_AIF_INFO]);
    aif_list = (struct aif_info*)blobmsg_data(tb[RETURN_AIF_LIST]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_get_aif_info_list(struct aif_info *aif_list, size_t *num_aif_info) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmGetAifInfoListSize_aif_info", num_aif_info,0);
    //blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmGetAifInfoListSize_aif_list", aif_list, sizeof(*aif_list));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmGetAifInfoList", b.head, agm_get_aif_info_list_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// ------------ agm_get_aif_info_list_size ---------------

void agm_get_aif_info_list_size_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    size_t *num_aif_info;

    agm_get_aif_info_list_size_return_policy[RETURN_AIF_INFO].name = "agm_get_aif_info_list_size_aif_info";
    agm_get_aif_info_list_size_return_policy[RETURN_AIF_INFO].type = BLOBMSG_TYPE_UNSPEC;
    agm_get_aif_info_list_size_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_get_aif_info_list_size_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_get_aif_info_list_size_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_AIF_INFO]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    num_aif_info = (size_t*)blobmsg_data(tb[RETURN_AIF_INFO]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_get_aif_info_list_size(size_t *num_aif_info) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    //blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmGetAifInfoListSize_aif_info", num_aif_info, 0);

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n",__func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmGetAifInfoListSize", b.head, agm_get_aif_info_list_size_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n",__func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// --------------- agm_set_params_with_tag -----------------

void agm_set_params_with_tag_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id, aif_id;
    struct agm_tag_config *tag_config;

    agm_set_params_with_tag_return_policy[RETURN_SESSION_ID].name = "agm_set_params_with_tag_session_id";
    agm_set_params_with_tag_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_set_params_with_tag_return_policy[RETURN_AIF_ID].name = "agm_set_params_with_tag_aif_id";
    agm_set_params_with_tag_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_set_params_with_tag_return_policy[RETURN_TAG_CONFIG].name = "agm_set_params_with_tag_tag_config";
    agm_set_params_with_tag_return_policy[RETURN_TAG_CONFIG].type = BLOBMSG_TYPE_UNSPEC;
    agm_set_params_with_tag_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_set_params_with_tag_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_set_params_with_tag_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_AIF_ID] || !tb[RETURN_TAG_CONFIG]) {
        AGM_LOGE("%s No return code received from server\n",__func__);
        return;
    }

    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    tag_config = (struct agm_tag_config*)blobmsg_data(tb[RETURN_TAG_CONFIG]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_set_params_with_tag(uint32_t session_id,
                            uint32_t aif_id,
                            struct agm_tag_config *tag_config){
    uint32_t id;
    AGM_LOGD("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSetParamsWithTag_session_id", session_id);
    blobmsg_add_u32(&b, "AgmSetParamsWithTag_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSetParamsWithTag_tag_config", tag_config, 0);

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n",__func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSetParamsWithTag", b.head, agm_set_params_with_tag_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// ----------- agm_session_set_loopback ----------

void agm_session_set_loopback_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t capture_session_id, playback_session_id;
    bool state;

    agm_session_set_loopback_return_policy[RETURN_CAPTURE_SESSION_ID].name = "agm_session_set_loopback_capture_session_id";
    agm_session_set_loopback_return_policy[RETURN_CAPTURE_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_set_loopback_return_policy[RETURN_PLAYBACK_SESSION_ID].name = "agm_session_set_loopback_playback_session_id";
    agm_session_set_loopback_return_policy[RETURN_PLAYBACK_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_set_loopback_return_policy[RETURN_STATE].name = "agm_session_set_loopback_state";
    agm_session_set_loopback_return_policy[RETURN_STATE].type = BLOBMSG_TYPE_BOOL;
    agm_session_set_loopback_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_set_loopback_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_set_loopback_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_CAPTURE_SESSION_ID] || !tb[RETURN_PLAYBACK_SESSION_ID] || !tb[RETURN_STATE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    capture_session_id = blobmsg_get_u32(tb[RETURN_CAPTURE_SESSION_ID]);
    playback_session_id = blobmsg_get_u32(tb[RETURN_PLAYBACK_SESSION_ID]);
    state = blobmsg_get_bool(tb[RETURN_STATE]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_set_loopback(uint32_t capture_session_id,
                             uint32_t playback_session_id,
                                         bool state)  {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionSetLoopback_capture_session_id", capture_session_id);
    blobmsg_add_u32(&b, "AgmSessionSetLoopback_playback_session_id", playback_session_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_BOOL, "AgmSessionSetLoopback_state", &state, sizeof(state));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n",__func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionSetLoopback", b.head, agm_session_set_loopback_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n",__func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// --------------- agm_session_set_metadata -------------------

void agm_session_set_metadata_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id, size;
    uint8_t metadata;

    agm_session_set_metadata_return_policy[RETURN_SESSION_ID].name = "agm_session_set_metadata_session_id";
    agm_session_set_metadata_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_set_metadata_return_policy[RETURN_METADATA].name = "agm_session_set_metadata_metadata";
    agm_session_set_metadata_return_policy[RETURN_METADATA].type = BLOBMSG_TYPE_INT8;
    agm_session_set_metadata_return_policy[RETURN_SIZE].name = "agm_session_set_metadata_size";
    agm_session_set_metadata_return_policy[RETURN_SIZE].type = BLOBMSG_TYPE_INT32;
    agm_session_set_metadata_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_set_metadata_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_set_metadata_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SIZE] || !tb[RETURN_METADATA] || !tb[RETURN_SESSION_ID]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    size = blobmsg_get_u32(tb[RETURN_SIZE]);
    metadata = blobmsg_get_u8(tb[RETURN_METADATA]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_set_metadata(uint32_t session_id,
                             uint32_t size, uint8_t *metadata)  {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionSetMetadata_session_id", session_id);
    blobmsg_add_u32(&b, "AgmSessionSetMetadata_size", size);
    blobmsg_add_u8(&b, "AgmSessionSetMetadata_metadata", (*metadata));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n",__func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionSetMetadata", b.head, agm_session_set_metadata_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n",__func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// -------------- agm_session_aif_set_metadata --------------

void agm_session_aif_set_metadata_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id, aif_id, size;
    uint8_t metadata;

    agm_session_aif_set_metadata_return_policy[RETURN_AIF_ID].name = "agm_session_aif_set_metadata_aif_id";
    agm_session_aif_set_metadata_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_set_metadata_return_policy[RETURN_SESSION_ID].name = "agm_session_aif_set_metadata_session_id";
    agm_session_aif_set_metadata_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_set_metadata_return_policy[RETURN_METADATA].name = "agm_session_aif_set_metadata_metadata";
    agm_session_aif_set_metadata_return_policy[RETURN_METADATA].type = BLOBMSG_TYPE_INT8;
    agm_session_aif_set_metadata_return_policy[RETURN_SIZE].name = "agm_session_aif_set_metadata_size";
    agm_session_aif_set_metadata_return_policy[RETURN_SIZE].type = BLOBMSG_TYPE_INT32;
    agm_session_aif_set_metadata_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_aif_set_metadata_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_aif_set_metadata_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_AIF_ID] || !tb[RETURN_SIZE] || !tb[RETURN_METADATA] || !tb[RETURN_SESSION_ID]) {
        AGM_LOGE("%s No return code received from server\n",__func__);
        return;
    }
    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    size = blobmsg_get_u32(tb[RETURN_SIZE]);
    metadata = blobmsg_get_u8(tb[RETURN_METADATA]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_aif_set_metadata(uint32_t session_id,
                                 uint32_t aif_id,
                                 uint32_t size, uint8_t *metadata)  {
    uint32_t id;

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionAifSetMetadata_session_id", session_id);
    blobmsg_add_u32(&b, "AgmSessionAifSetMetadata_aif_id", aif_id);
    blobmsg_add_u32(&b, "AgmSessionAifSetMetadata_size", size);
    blobmsg_add_u8(&b, "AgmSessionAifSetMetadata_metadata", (*metadata));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionAifSetMetadata", b.head, agm_session_aif_set_metadata_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// ------------ agm_aif_set_metadata --------------

void agm_aif_set_metadata_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t aif_id, size;
    uint8_t metadata;

    agm_aif_set_metadata_return_policy[RETURN_AIF_ID].name = "agm_aif_set_metadata_aif_id";
    agm_aif_set_metadata_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_aif_set_metadata_return_policy[RETURN_METADATA].name = "agm_aif_set_metadata_metadata";
    agm_aif_set_metadata_return_policy[RETURN_METADATA].type = BLOBMSG_TYPE_INT8;
    agm_aif_set_metadata_return_policy[RETURN_SIZE].name = "agm_aif_set_metadata_size";
    agm_aif_set_metadata_return_policy[RETURN_SIZE].type = BLOBMSG_TYPE_INT32;
    agm_aif_set_metadata_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_aif_set_metadata_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_aif_set_metadata_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_AIF_ID] || !tb[RETURN_SIZE] || !tb[RETURN_METADATA]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    size = blobmsg_get_u32(tb[RETURN_SIZE]);
    metadata = blobmsg_get_u8(tb[RETURN_METADATA]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_aif_set_metadata(uint32_t aif_id,
                         uint32_t size, uint8_t *metadata) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmAifSetMetadata_aif_id", aif_id);
    blobmsg_add_u32(&b, "AgmAifSetMetadata_size", size);
    blobmsg_add_u8(&b, "AgmAifSetMetadata_metadata", (*metadata));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmAifSetMetadata", b.head, agm_aif_set_metadata_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// ----------- agm_aif_set_media_config -----------------

void agm_aif_set_media_config_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t aif_id;
    struct agm_media_config *media_config;
    agm_aif_set_media_config_return_policy[RETURN_AIF_ID].name = "agm_aif_set_media_config_aif_id";
    agm_aif_set_media_config_return_policy[RETURN_AIF_ID].type = BLOBMSG_TYPE_INT32;
    agm_aif_set_media_config_return_policy[RETURN_MEDIA_CONFIG].name = "agm_aif_set_media_config_media_config";
    agm_aif_set_media_config_return_policy[RETURN_MEDIA_CONFIG].type = BLOBMSG_TYPE_UNSPEC;
    agm_aif_set_media_config_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_aif_set_media_config_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_aif_set_media_config_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_AIF_ID] || !tb[RETURN_MEDIA_CONFIG]) {
        AGM_LOGE("%s No return code received from server\n",__func__);
        return;
    }

    aif_id = blobmsg_get_u32(tb[RETURN_AIF_ID]);
    media_config = (struct agm_media_config*)blobmsg_data(tb[RETURN_MEDIA_CONFIG]);
    AGM_LOGE("%s Recieving from the server\n", __func__);
}

int agm_aif_set_media_config(uint32_t aif_id,
                             struct agm_media_config *media_config) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmAifSetMediaConfig_aif_id", aif_id);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmAifSetMediaConfig_media_config", media_config, sizeof(*media_config));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmAifSetMediaConfig", b.head, agm_aif_set_media_config_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n",__func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// ------------- agm_get_buffer_timestamp ------------

void agm_get_buffer_timestamp_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id;
    uint64_t timestamp;

    agm_get_buffer_timestamp_return_policy[RETURN_SESSION_ID].name = "agm_get_buffer_timestamp_session_id";
    agm_get_buffer_timestamp_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_get_buffer_timestamp_return_policy[RETURN_TIMESTAMP].name = "agm_get_buffer_timestamp_timestamp";
    agm_get_buffer_timestamp_return_policy[RETURN_TIMESTAMP].type = BLOBMSG_TYPE_INT64;
    agm_get_buffer_timestamp_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_get_buffer_timestamp_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_get_buffer_timestamp_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_ID] || !tb[RETURN_TIMESTAMP]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    session_id = blobmsg_get_u64(tb[RETURN_SESSION_ID]);
    timestamp = blobmsg_get_u64(tb[RETURN_TIMESTAMP]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_get_buffer_timestamp(uint32_t session_id, uint64_t *timestamp) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmGetBufferTimestamp_session_id", session_id);
    //blobmsg_add_u64(&b, "AgmGetBufferTimestamp_timestamp", (*timestamp));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmGetBufferTimestamp", b.head, agm_get_buffer_timestamp_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// ----------- agm_get_hw_processed_buff_cnt --------------

void agm_get_hw_processed_buff_cnt_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg){
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;
    enum direction *dir;
    size_t *buf_count;

    agm_get_hw_processed_buff_cnt_return_policy[RETURN_SESSION_HANDLE].name = "agm_get_hw_processed_buff_cnt_session_handle" ;
    agm_get_hw_processed_buff_cnt_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_get_hw_processed_buff_cnt_return_policy[RETURN_DIRECTION].name = "agm_get_hw_processed_buff_cnt_direction";
    agm_get_hw_processed_buff_cnt_return_policy[RETURN_DIRECTION].type = BLOBMSG_TYPE_UNSPEC;
    agm_get_hw_processed_buff_cnt_return_policy[RETURN_BYTE_COUNT].name = "agm_get_hw_processed_buff_cnt_buffer";
    agm_get_hw_processed_buff_cnt_return_policy[RETURN_BYTE_COUNT].type = BLOBMSG_TYPE_UNSPEC;
    agm_get_hw_processed_buff_cnt_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_get_hw_processed_buff_cnt_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_get_hw_processed_buff_cnt_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if( !tb[RETURN_SESSION_HANDLE] || !tb[RETURN_DIRECTION] || !tb[RETURN_BYTE_COUNT]){
        AGM_LOGE("%s No return code received from server\n",__func__);
        return;
    }

    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    dir = (enum direction*) blobmsg_data(tb[RETURN_DIRECTION]);
    buf_count = (size_t*)blobmsg_data(tb[RETURN_BYTE_COUNT]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}
size_t agm_get_hw_processed_buff_cnt(uint64_t handle, enum direction dir) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64( &b, "AgmGetHwProcessedBufCount_session_handle" , handle);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmGetHwProcessedBufCount_direction", &dir, sizeof(dir));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmGetHwProcessedBufCount", b.head, agm_get_hw_processed_buff_cnt_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n",__func__);
        return -1;
    }
    AGM_LOGE("%s called the server\n", __func__);
    return 0;
}

// ------- agm_get_session_time ---------

void agm_get_session_time_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle, timestamp;

    agm_get_session_time_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_get_time_session_handle";
    agm_get_session_time_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_get_session_time_return_policy[RETURN_TIMESTAMP].name = "agm_session_get_time_timestamp";
    agm_get_session_time_return_policy[RETURN_TIMESTAMP].type = BLOBMSG_TYPE_INT64;
    agm_get_session_time_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_get_session_time_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_get_session_time_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_SESSION_HANDLE] || !tb[RETURN_TIMESTAMP]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    timestamp = blobmsg_get_u64(tb[RETURN_TIMESTAMP]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_get_session_time(uint64_t handle, uint64_t *timestamp) {
    uint32_t id;
    AGM_LOGE("%s\n",__func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionGetTime_session_handle", handle);
    //blobmsg_add_u64(&b, "AgmSessionGetTime_timestamp", (*timestamp));

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionGetTime", b.head, agm_get_session_time_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n",__func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// ------- agm_session_eos ---------

void agm_session_eos_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;

    agm_session_eos_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_eos_session_handle";
    agm_session_eos_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_eos_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_eos_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_eos_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if (!tb[RETURN_SESSION_HANDLE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    AGM_LOGE("%s Recieving from the server\n", __func__);
}

int agm_session_eos(uint64_t handle) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionEos_session_handle", handle);

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionEos", b.head, agm_session_eos_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Errror in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

// --------- agm_session_set_config ----------

void agm_session_set_config_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;
    struct agm_session_config *session_config;
    struct agm_media_config *media_config;
    struct agm_buffer_config *buffer_config;

    agm_session_set_config_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_set_config_session_handle";
    agm_session_set_config_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_set_config_return_policy[RETURN_SESSION_CONFIG].name = "agm_session_set_config_session_config";
    agm_session_set_config_return_policy[RETURN_SESSION_CONFIG].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_set_config_return_policy[RETURN_MEDIA_CONFIG].name = "agm_session_set_config_media_config";
    agm_session_set_config_return_policy[RETURN_MEDIA_CONFIG].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_set_config_return_policy[RETURN_BUFFER_CONFIG].name = "agm_session_set_config_buffer_config";
    agm_session_set_config_return_policy[RETURN_BUFFER_CONFIG].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_set_config_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_set_config_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_set_config_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if(!tb[RETURN_BUFFER_CONFIG] || !tb[RETURN_MEDIA_CONFIG] || !tb[RETURN_SESSION_CONFIG] || !tb[RETURN_SESSION_HANDLE]){
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    session_config = (struct agm_session_config*)blobmsg_data(tb[RETURN_MEDIA_CONFIG]);
    media_config = (struct agm_media_config*) blobmsg_data(tb[RETURN_MEDIA_CONFIG]);
    buffer_config = (struct agm_buffer_config*)blobmsg_data(tb[RETURN_SESSION_CONFIG]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_set_config(uint64_t handle,
                    struct agm_session_config *session_config,
                    struct agm_media_config *media_config,
                    struct agm_buffer_config *buffer_config) {
    uint32_t id;
    AGM_LOGE("%s\n",__func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionSetConfig_session_handle", handle);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionSetConfig_session_config", session_config, sizeof(*session_config));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionSetConfig_media_config", media_config, sizeof(*media_config));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionSetConfig_buffer_config", buffer_config, sizeof(*buffer_config));

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionSetConfig", b.head, agm_session_set_config_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

void agm_session_write_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg){
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;
    void* buf;
    size_t *byte_count;

    agm_session_write_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_write_session_handle";
    agm_session_write_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_write_return_policy[RETURN_BUFFER].name = "agm_session_write_buffer";
    agm_session_write_return_policy[RETURN_BUFFER].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_write_return_policy[RETURN_BYTE_COUNT].name = "agm_session_write_byte_count";
    agm_session_write_return_policy[RETURN_BYTE_COUNT].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_write_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_write_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_write_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if( !tb[RETURN_BUFFER] || !tb[RETURN_BYTE_COUNT] || !tb[RETURN_SESSION_HANDLE] ){
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    buf = blobmsg_data(tb[RETURN_BUFFER]);
    byte_count = (size_t* ) blobmsg_data(tb[RETURN_BYTE_COUNT]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_write(uint64_t handle, void *buf, size_t *byte_count){
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionWrite_session_handle", handle);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionWrite_buffer", buf, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionWrite_byte_count", byte_count, sizeof(*byte_count));

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionWrite", b.head, agm_session_write_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

void agm_session_read_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg){
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;
    void* buf;
    size_t *byte_count;

    agm_session_read_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_read_session_handle";
    agm_session_read_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_read_return_policy[RETURN_BUFFER].name = "agm_session_read_buffer";
    agm_session_read_return_policy[RETURN_BUFFER].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_read_return_policy[RETURN_BYTE_COUNT].name = "agm_session_read_byte_count";
    agm_session_read_return_policy[RETURN_BYTE_COUNT].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_read_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_read_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_read_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if( !tb[RETURN_BUFFER] || !tb[RETURN_BYTE_COUNT] || !tb[RETURN_SESSION_HANDLE] ){
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    buf = blobmsg_data(tb[RETURN_BUFFER]);
    byte_count = (size_t* ) blobmsg_data(tb[RETURN_BYTE_COUNT]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_read(uint64_t handle, void *buf, size_t *byte_count){
    uint32_t id;
    AGM_LOGE("%s\n",__func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionRead_session_handle", handle);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionRead_buffer", buf, 0);
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionRead_byte_count", byte_count, sizeof(*byte_count));

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionRead", b.head, agm_session_read_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

void agm_session_pause_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;

    agm_session_pause_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_pause_session_handle";
    agm_session_pause_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_read_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_read_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_pause_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if (!tb[RETURN_SESSION_HANDLE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_pause(uint64_t handle) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionPause_session_handle", handle);

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling from the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionPause", b.head, agm_session_pause_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

void agm_session_resume_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;

    agm_session_resume_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_resume_session_handle";
    agm_session_resume_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_resume_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_resume_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_resume_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if (!tb[RETURN_SESSION_HANDLE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_resume(uint64_t handle) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionResume_session_handle", handle);

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionResume", b.head, agm_session_resume_cb, 0, 5000);
    if(ret) {
        AGM_LOGD("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s called the server\n", __func__);
    return 0;
}

void agm_session_prepare_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;

    agm_session_prepare_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_prepare_session_handle";
    agm_session_prepare_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_prepare_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_prepare_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_prepare_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if (!tb[RETURN_SESSION_HANDLE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_prepare(uint64_t handle) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionPrepare_session_handle", handle);

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionPrepare", b.head, agm_session_prepare_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s called the server\n", __func__);;
    return 0;
}

void agm_session_start_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;

    agm_session_start_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_start_session_handle";
    agm_session_start_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_start_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_start_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_start_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if (!tb[RETURN_SESSION_HANDLE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_start(uint64_t handle) {
    uint32_t id;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionStart_session_handle", handle);

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionStart", b.head, agm_session_start_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Called the server\n", __func__);
    return 0;
}

void agm_session_stop_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;

    agm_session_stop_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_stop_session_handle";
    agm_session_stop_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_stop_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_stop_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_stop_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if (!tb[RETURN_SESSION_HANDLE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_stop(uint64_t handle) {
    uint32_t id;
    AGM_LOGD("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionStop_session_handle", handle);

    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionStop", b.head, agm_session_stop_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }
    AGM_LOGE("%s called the server\n", __func__);
    return 0;
}

void agm_session_close_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg) {
    struct blob_attr *tb[__RETURN_MAX];
    uint64_t handle;
    AGM_LOGE("%s\n", __func__);

    agm_session_close_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_close_session_handle";
    agm_session_close_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_close_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_close_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_close_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if (!tb[RETURN_SESSION_HANDLE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }
    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_close(uint64_t handle) {
    uint32_t id;
    agm_client_session_data *ses_data = (agm_client_session_data *)g_malloc0(sizeof(agm_client_session_data));
    ses_data = (agm_client_session_data *)handle;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u64(&b, "AgmSessionClose_session_handle", handle);
    if (ubus_lookup_id(ctx, "agm_ubus_session", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s calling the server\n", __func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionClose", b.head, agm_session_close_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }

    AGM_LOGE("%s hash table\n", __func__);
    g_hash_table_remove(ses_hash_table, GINT_TO_POINTER(ses_data->session_id));

    AGM_LOGE("%s called the server\n", __func__);
    return 0;
}

void agm_session_open_cb(struct ubus_request *req,
                    int type, struct blob_attr *msg)
{
    struct blob_attr *tb[__RETURN_MAX];
    uint32_t session_id;
    uint64_t handle;
    enum agm_session_mode* sess_mode;

    agm_session_open_return_policy[RETURN_SESSION_ID].name = "agm_session_open_session_id";
    agm_session_open_return_policy[RETURN_SESSION_ID].type = BLOBMSG_TYPE_INT32;
    agm_session_open_return_policy[RETURN_SESSION_MODE].name = "agm_session_open_session_mode";
    agm_session_open_return_policy[RETURN_SESSION_MODE].type = BLOBMSG_TYPE_UNSPEC;
    agm_session_open_return_policy[RETURN_SESSION_HANDLE].name = "agm_session_open_session_handle";
    agm_session_open_return_policy[RETURN_SESSION_HANDLE].type = BLOBMSG_TYPE_INT64;
    agm_session_open_return_policy[RETURN_ERROR].name = "agm_ubus_error";
    agm_session_open_return_policy[RETURN_ERROR].type = BLOBMSG_TYPE_INT32;

    AGM_LOGE("%s Recieving from the server\n", __func__);
    blobmsg_parse(agm_session_open_return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

    if(tb[RETURN_ERROR]){
        AGM_LOGE("%s Error in Recieving data from Server", __func__);
        return;
    }

    if (!tb[RETURN_SESSION_ID] || !tb[RETURN_SESSION_HANDLE] || !tb[RETURN_SESSION_MODE]) {
        AGM_LOGE("%s No return code received from server\n", __func__);
        return;
    }

    session_id = blobmsg_get_u32(tb[RETURN_SESSION_ID]);
    handle = blobmsg_get_u64(tb[RETURN_SESSION_HANDLE]);
    sess_mode = (enum agm_session_mode* )blobmsg_data(tb[RETURN_SESSION_MODE]);
    AGM_LOGE("%s Recieved from the server\n", __func__);
}

int agm_session_open(uint32_t session_id, enum agm_session_mode sess_mode, uint64_t *handle) {
    uint32_t id;
    agm_client_session_data *ses_data = NULL;
    AGM_LOGE("%s\n", __func__);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "AgmSessionOpen_session_id", session_id);
    blobmsg_add_u64( &b, "AgmSessionOpen_session_handle" , (*handle));
    blobmsg_add_field(&b, BLOBMSG_TYPE_UNSPEC, "AgmSessionOpen_session_mode", &sess_mode, sizeof(sess_mode));

    if (ubus_lookup_id(ctx, "agm_ubus_module", &id)) {
        AGM_LOGE("%s Failed to look up test object\n", __func__);
        return -1;
    }

    AGM_LOGE("%s Calling the server\n",__func__);
    int ret = ubus_invoke(ctx, id, "AgmSessionOpen", b.head, agm_session_open_cb, 0, 5000);
    if(ret) {
        AGM_LOGE("%s Error in calling Server Function\n", __func__);
        return -1;
    }

    AGM_LOGE("%sHash Table\n", __func__);
    if(g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id)) == NULL ) {
        ses_data = (agm_client_session_data*)g_hash_table_lookup(ses_hash_table, GINT_TO_POINTER(session_id));
        ses_data = (agm_client_session_data *)g_malloc0(sizeof(agm_client_session_data));
        ses_data -> session_id = session_id;
        ses_data -> ctx = ctx;
        g_hash_table_insert(ses_hash_table, GINT_TO_POINTER(session_id), ses_data);
    }
    AGM_LOGE("%s called the server\n", __func__);
    return 0;
}

int agm_init() {
    int rc = 0;
    AGM_LOGD("%s\n", __func__);
    rc = initialize_module_data();
    return rc;
}

void agm_deint() {
    ubus_free(ctx);
    uloop_done();
}