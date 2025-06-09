/**
 ** Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 ** SPDX-License-Identifier: BSD-3-Clause-Clear
 **/

#include <agm_mixer_plugin.h>
#include <agm/codec/codec_interface.h>
#include <kvh2xml.h>

#define ROUTE_NONE (0)
#define CODEC_ROUTE_NUM_CKVS (2)
#define MAX_NUM_CDC_MODULE (5)
// Payload of CDC module list - MID:PID:SIZE:NUM_MODULES:MODULE_IDS
#define PAYLOAD_SIZE_CDC_MODULE_LIST (sizeof(uint32_t) + sizeof(uint32_t)\
               + sizeof(uint32_t) + sizeof(uint32_t) \
               + (MAX_NUM_CDC_MODULE * sizeof(uint32_t)))
#define MODULE_ID_DRIVER_MODULE_LIST                              0x0C000040
#define Q6_CODEC_MODULE_ID 0x0C000040

typedef struct _DRIVER_MODULE_ID {
    uint32_t          Module_ID;
} DRIVER_MODULE_ID;

typedef struct _DRIVER_MODULE_LIST {
    uint32_t                      NumberOfModules;
    DRIVER_MODULE_ID    ModuleList[0];
}DRIVER_MODULE_LIST;

enum {
    BE_CTL_NAME_ENABLE_CODEC_ROUTE = 0,
    BE_CTL_NAME_DISABLE_CODEC_ROUTE,
};

/* strings should be at the index as per the #defines */
static char *amp_non_alsa_be_ctl_name_extn[] = {
    "enableCodecRoute",
    "disableCodecRoute"
};

enum {
    SND_DEVICE_IDX_SPEAKER = 0,
    SND_DEVICE_IDX_SPEAKER_PROTECTED,
    SND_DEVICE_IDX_SPEAKER_MIC,
    SND_DEVICE_IDX_HANDSET_MIC,
    SND_DEVICE_IDX_DMIC2,
    SND_DEVICE_IDX_DMIC3,
    SND_DEVICE_IDX_BT_A2DP,
    SND_DEVICE_IDX_BT_A2DP_MIC,
    SND_DEVICE_IDX_BT_SCO_OUT,
    SND_DEVICE_IDX_BT_SCO_MIC,
    SND_DEVICE_IDX_BT_BLE_OUT,
    SND_DEVICE_IDX_BT_BLE_MIC,
    SND_DEVICE_IDX_HAPTICS,
    SND_DEVICE_IDX_MAX,
};

/* strings should be at the index as per SND_DEVICE_* enum  */
static char *cmp_snd_device_names[] = {
    [SND_DEVICE_IDX_SPEAKER]           = "speaker",
    [SND_DEVICE_IDX_SPEAKER_PROTECTED] = "speaker-protected",
    [SND_DEVICE_IDX_SPEAKER_MIC]       = "speaker-mic",
    [SND_DEVICE_IDX_HANDSET_MIC]       = "handset-mic",
    [SND_DEVICE_IDX_DMIC2]             = "dmic2",
    [SND_DEVICE_IDX_DMIC3]             = "dmic3",
    [SND_DEVICE_IDX_BT_A2DP]           = "bt-a2dp",
    [SND_DEVICE_IDX_BT_A2DP_MIC]       = "bt-a2dp-mic",
    [SND_DEVICE_IDX_BT_SCO_OUT]        = "bt-sco",
    [SND_DEVICE_IDX_BT_SCO_MIC]        = "bt-sco-mic",
    [SND_DEVICE_IDX_BT_BLE_OUT]        = "bt-ble",
    [SND_DEVICE_IDX_BT_BLE_MIC]        = "bt-ble-mic",
    [SND_DEVICE_IDX_HAPTICS]           = "haptics-dev",
};

uint32_t snd_device_enum_to_route_map[SND_DEVICE_IDX_MAX] = {
    [SND_DEVICE_IDX_SPEAKER]     = ROUTE_SPEAKER,
    [SND_DEVICE_IDX_SPEAKER_MIC] = ROUTE_SPEAKER_MIC,
    [SND_DEVICE_IDX_HANDSET_MIC] = ROUTE_HANDSET_MIC,
    [SND_DEVICE_IDX_DMIC2]       = ROUTE_DMIC2,
    [SND_DEVICE_IDX_DMIC3]       = ROUTE_DMIC3,
    [SND_DEVICE_IDX_BT_A2DP]     = ROUTE_BT_A2DP,
    [SND_DEVICE_IDX_BT_A2DP_MIC] = ROUTE_BT_A2DP_MIC,
    [SND_DEVICE_IDX_BT_SCO_OUT]  = ROUTE_BT_SCO,
    [SND_DEVICE_IDX_BT_SCO_MIC]  = ROUTE_BT_SCO_MIC,
    [SND_DEVICE_IDX_BT_BLE_OUT]  = ROUTE_BT_BLE,
    [SND_DEVICE_IDX_BT_BLE_MIC]  = ROUTE_BT_BLE_MIC,
    [SND_DEVICE_IDX_HAPTICS]     = ROUTE_HAPTICS,
};

struct snd_value_enum codec_routing_enum;

static void cmp_free_codec_route(struct codec_module_payload_list *mp_list);

#ifdef HAS_NON_ALSA_DAI
int amp_get_non_alsa_be_ctl_count(struct amp_priv *amp_priv)
{
    struct amp_dev_info *rx_adi = &amp_priv->rx_be_devs;
    struct amp_dev_info *tx_adi = &amp_priv->tx_be_devs;
    int count, ctl_per_be;

    ctl_per_be = (int)ARRAY_SIZE(amp_non_alsa_be_ctl_name_extn);

    count = 0;

    /* minus 1 is needed to ignore the ZERO string (name) */
    count += (rx_adi->non_alsa_aif_info_count) * ctl_per_be;
    count += (tx_adi->non_alsa_aif_info_count) * ctl_per_be;

    return count;
}

static int cmp_enable_codec_route_control_get(struct mixer_plugin *plugin __unused,
                struct snd_control *ctl, struct snd_ctl_elem_value *ev)
{
    /* TODO: AGM needs to provide this in a API */
    AGM_LOGV("%s: enter\n", __func__);
    return 0;
}

static int cmp_disable_codec_route_control_get(struct mixer_plugin *plugin __unused,
                struct snd_control *ctl, struct snd_ctl_elem_value *ev)
{
    /* TODO: AGM needs to provide this in a API */
    AGM_LOGV("%s: enter\n", __func__);
    return 0;
}

static int cmp_get_codec_module_payload(struct agm_cal_config *cal_config,
                        struct codec_module_payload_list *mp_list)
{
    uint32_t driver_module = MODULE_ID_DRIVER_MODULE_LIST;
    size_t driver_list_payload_size = PAYLOAD_SIZE_CDC_MODULE_LIST;
    uint8_t module_list_blob[PAYLOAD_SIZE_CDC_MODULE_LIST];
    DRIVER_MODULE_LIST *driver_module_list;
    struct codec_module_payload *mp = NULL;
    void *payload = NULL;
    int32_t ret = 0;

    memset(mp_list, 0, sizeof(struct codec_module_payload_list));
    ret = agm_get_driver_data(driver_module, cal_config,
                &module_list_blob, &driver_list_payload_size);
    if (ret)
        goto exit;

    driver_module_list = (DRIVER_MODULE_LIST *)(uint32_t*)module_list_blob + 3;
    mp_list->num_mp = driver_module_list->NumberOfModules;
    mp_list->mp_arr = (struct codec_module_payload*)calloc(sizeof(struct codec_module_payload) * mp_list->num_mp, 1);
    mp_list->total_payload_size = 0;

    for (int i = 0; i < mp_list->num_mp; i++)
    {
        mp = mp_list->mp_arr + i;
        mp->mid = driver_module_list->ModuleList[i].Module_ID;
        ret = agm_get_driver_data(mp->mid, cal_config, NULL, &mp->size);
        if (ret)
            goto exit;

        mp->data = calloc(1, mp->size);
        if (!mp->data)
            goto exit;

        ret = agm_get_driver_data(mp->mid, cal_config, mp->data, &mp->size);
        ret = 0;
        if (ret)
            goto exit;
        mp_list->total_payload_size += mp->size;
    }

exit:
    if (ret)
        cmp_free_codec_route(mp_list);
    return ret;
}

static void cmp_free_codec_route(struct codec_module_payload_list *mp_list)
{
    struct codec_module_payload *mp = NULL;
    if (!mp_list)
        return;

    for (int i = 0; i < mp_list->num_mp; i++)
    {
        mp = mp_list->mp_arr + i;
        if (mp->data)
            free(mp->data);
        mp->data = NULL;
    }
    free(mp_list->mp_arr);
    mp_list->mp_arr = NULL;
    mp_list->num_mp = 0;
    mp_list->total_payload_size = 0;

    return;
}

static int cmp_enable_codec_route_control_put(struct mixer_plugin *plugin __unused,
               struct snd_control *ctl, struct snd_ctl_elem_value *ev)
{
    struct amp_dev_info *adi = ctl->private_data;
    uint32_t idx = ctl->private_value;
    uint32_t pcm = adi->non_alsa_aif_info_arr[idx].pcm;
    uint32_t snd_device_idx = ev->value.enumerated.item[0];
    uint32_t route_value = ROUTE_NONE;
    struct agm_cal_config *cal_config = NULL;
    int cal_config_size = 0;
    struct codec_module_payload_list mp_list;
    int ret = 0;

    if (snd_device_idx >= SND_DEVICE_IDX_MAX) {
        AGM_LOGE("%s: invalid control value %d\n", __func__, snd_device_idx);
        ret = -EINVAL;
        goto done;
    }

    route_value = snd_device_enum_to_route_map[snd_device_idx];
    AGM_LOGI("%s: pcm_id = %u, snd_device = %s, route_val = 0x%X\n", __func__,
        pcm, cmp_snd_device_names[snd_device_idx], route_value);

    if (route_value == ROUTE_NONE) {
        ret = -EINVAL;
        goto done;
    }

    cal_config_size = sizeof(struct agm_cal_config) +
                        (sizeof(struct agm_key_value) * CODEC_ROUTE_NUM_CKVS);
    cal_config = (struct agm_cal_config*)calloc(1, cal_config_size);
    cal_config->num_ckvs = CODEC_ROUTE_NUM_CKVS;
    cal_config->kv[0].key = ROUTE_ID;
    cal_config->kv[0].value = route_value;
    cal_config->kv[1].key = ROUTE_ACTION;
    cal_config->kv[1].value = ROUTE_ENABLE;

     memset(&mp_list, 0, sizeof(mp_list));
     ret = cmp_get_codec_module_payload(cal_config, &mp_list);
     if (ret)
        goto done;

     ret = cdc_route_endpoint(pcm, &mp_list, true);

done:

    cmp_free_codec_route(&mp_list);

    if (cal_config)
        free(cal_config);

    AGM_LOGI("%s: ret %d\n", __func__, ret);
    return ret;
}

static int cmp_disable_codec_route_control_put(struct mixer_plugin *plugin __unused,
               struct snd_control *ctl, struct snd_ctl_elem_value *ev)
{
    struct amp_dev_info *adi = ctl->private_data;
    uint32_t idx = ctl->private_value;
    uint32_t pcm = adi->non_alsa_aif_info_arr[idx].pcm;
    uint32_t snd_device_idx = ev->value.enumerated.item[0];
    uint32_t route_value = ROUTE_NONE;
    struct agm_cal_config *cal_config = NULL;
    int cal_config_size = 0;
    struct codec_module_payload_list mp_list;
    int ret = 0;

    if (snd_device_idx >= SND_DEVICE_IDX_MAX) {
        AGM_LOGE("%s: invalid control value %d\n", __func__, snd_device_idx);
        ret = -EINVAL;
        goto done;
    }

    route_value = snd_device_enum_to_route_map[snd_device_idx];
    AGM_LOGI("%s: pcm_id = %u, snd_device = %s, route_val = 0x%X\n", __func__,
        pcm, cmp_snd_device_names[snd_device_idx], route_value);

    if (route_value == ROUTE_NONE) {
        ret = -EINVAL;
        goto done;
    }

    cal_config_size = sizeof(struct agm_cal_config) +
                        (sizeof(struct agm_key_value) * CODEC_ROUTE_NUM_CKVS);
    cal_config = (struct agm_cal_config*)calloc(1, cal_config_size);
    cal_config->num_ckvs = CODEC_ROUTE_NUM_CKVS;
    cal_config->kv[0].key = ROUTE_ID;
    cal_config->kv[0].value = route_value;
    cal_config->kv[1].key = ROUTE_ACTION;
    cal_config->kv[1].value = ROUTE_DISABLE;


     memset(&mp_list, 0, sizeof(mp_list));
     ret = cmp_get_codec_module_payload(cal_config, &mp_list);
     if (ret)
        goto done;

     ret = cdc_route_endpoint(pcm, &mp_list, false);

done:

    cmp_free_codec_route(&mp_list);

    if (cal_config)
        free(cal_config);

    return ret;
}

static void amp_create_enable_codec_route_ctl(struct amp_priv *amp_priv,
                char *pname, int ctl_idx, struct snd_value_enum *e,
                int pval, void *pdata)
{
    struct snd_control *ctl = AMP_PRIV_GET_CTL_PTR(amp_priv, ctl_idx);
    char *ctl_name = AMP_PRIV_GET_CTL_NAME_PTR(amp_priv, ctl_idx);

    snprintf(ctl_name, AIF_NAME_MAX_LEN + 16, "%s %s",
             pname, amp_non_alsa_be_ctl_name_extn[BE_CTL_NAME_ENABLE_CODEC_ROUTE]);
    AGM_LOGI("%s registered successfully\n", ctl_name);
    INIT_SND_CONTROL_ENUM(ctl, ctl_name, cmp_enable_codec_route_control_get,
                    cmp_enable_codec_route_control_put, e, pval, pdata);

}

static void amp_create_disable_codec_route_ctl(struct amp_priv *amp_priv,
                char *pname, int ctl_idx, struct snd_value_enum *e,
                int pval, void *pdata)
{
    struct snd_control *ctl = AMP_PRIV_GET_CTL_PTR(amp_priv, ctl_idx);
    char *ctl_name = AMP_PRIV_GET_CTL_NAME_PTR(amp_priv, ctl_idx);

    snprintf(ctl_name, AIF_NAME_MAX_LEN + 16, "%s %s",
             pname, amp_non_alsa_be_ctl_name_extn[BE_CTL_NAME_DISABLE_CODEC_ROUTE]);
    AGM_LOGI("%s registered successfully\n", ctl_name);
    INIT_SND_CONTROL_ENUM(ctl, ctl_name, cmp_disable_codec_route_control_get,
                    cmp_disable_codec_route_control_put, e, pval, pdata);

}

void cmp_form_codec_routing_enum(void)
{
    codec_routing_enum.items = SND_DEVICE_IDX_MAX;
    codec_routing_enum.texts = cmp_snd_device_names;
}

int amp_form_non_alsa_be_ctls(struct amp_priv *amp_priv, int ctl_idx, int ctl_cnt __unused)
{
    struct amp_dev_info *rx_adi = &amp_priv->rx_be_devs;
    struct amp_dev_info *tx_adi = &amp_priv->tx_be_devs;
    int i, non_alsa_idx = 0;

    cmp_form_codec_routing_enum();

    cdc_interface_init();

    for (i = 0; i < rx_adi->non_alsa_aif_info_count; i++) {
        non_alsa_idx = rx_adi->non_alsa_aif_info_arr[i].non_alsa_idx;
        amp_create_enable_codec_route_ctl(amp_priv, rx_adi->names[non_alsa_idx], ctl_idx++,
                        &codec_routing_enum, i, rx_adi);
        amp_create_disable_codec_route_ctl(amp_priv, rx_adi->names[non_alsa_idx], ctl_idx++,
                        &codec_routing_enum, i, rx_adi);
    }

    for (i = 0; i < tx_adi->non_alsa_aif_info_count; i++) {
        non_alsa_idx = tx_adi->non_alsa_aif_info_arr[i].non_alsa_idx;
        amp_create_enable_codec_route_ctl(amp_priv, tx_adi->names[non_alsa_idx], ctl_idx++,
                        &codec_routing_enum, i, tx_adi);
        amp_create_disable_codec_route_ctl(amp_priv, tx_adi->names[non_alsa_idx], ctl_idx++,
                        &codec_routing_enum, i, tx_adi);
    }

    return 0;
}

#endif
