/*
** Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
** Copyright (c) 2022, 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
**/

#ifndef _AGM_MIXER_PLUGIN_H_
#define _AGM_MIXER_PLUGIN_H_

/* agm_mixer.c all names (variable/functions) should have
   amp_ (Agm Mixer Plugin) */
#define LOG_TAG "PLUGIN: mixer"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <limits.h>
#include <linux/ioctl.h>

#include <sound/asound.h>

#include <tinyalsa/asoundlib.h>
#include <tinyalsa/mixer_plugin.h>

#include <agm/agm_api.h>
#include <snd-card-def.h>

#include <agm/agm_list.h>
#include <agm/utils.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_MIXER_PLUGIN
#include <log_utils.h>
#endif


#define ARRAY_SIZE(a)    \
    (sizeof(a) / sizeof(a[0]))

#define AMP_PRIV_GET_CTL_PTR(p, idx) \
    (p->ctls + idx)

#define AMP_PRIV_GET_CTL_NAME_PTR(p, idx) \
    (p->ctl_names[idx])

struct amp_get_param_info {
    void *get_param_payload;
    int get_param_payload_size;
};

struct amp_non_alsa_aif_info {
    int non_alsa_idx;
    int card;
    int pcm;
};

struct amp_dev_info {
    char **names;
    int *idx_arr;
    int count;
    struct snd_value_enum dev_enum;
    enum direction dir;

    /*
     * Mixer ctl data cache for
     * "pcm<id> metadata_control"
     * Unused for BE devs
     */
    int *pcm_mtd_ctl;

    /*
     * Mixer ctl data cache for
     * "pcm<id> getParam"
     * Unused for BE devs
     */
    struct amp_get_param_info *get_param_info;

    /*
     * Addtional info of Non alsa indexes in idx_arr
     * Unused for FE devs
     */
    struct amp_non_alsa_aif_info *non_alsa_aif_info_arr;

    /*
     * Number of non-alsa indexes in idx_arr
     * Unused for FE devs
     */
    int non_alsa_aif_info_count;
};

struct amp_be_group_info {
    char **names;
    int *idx_arr;
    int count;
};

struct amp_priv {
    unsigned int card;
    void *card_node;

    struct aif_info *aif_list;
    struct listnode events_list;
    struct listnode events_paramlist;

    struct amp_dev_info rx_be_devs;
    struct amp_dev_info tx_be_devs;
    struct amp_dev_info rx_pcm_devs;
    struct amp_dev_info tx_pcm_devs;
    struct amp_dev_info acdb_tunnels;

    struct amp_be_group_info group_be_devs;

    struct snd_control *ctls;
    char (*ctl_names)[AIF_NAME_MAX_LEN + 16];
    int ctl_count;

    struct snd_value_enum tx_be_enum;
    struct snd_value_enum rx_be_enum;

    event_callback event_cb;
    pthread_mutex_t lock;
};

struct event_params_node {
    uint32_t session_id;
    struct listnode node;
    struct agm_event_cb_params event_params;
};

struct mixer_plugin_event_data {
    struct ctl_event ev;
    struct listnode node;
};

int amp_form_non_alsa_be_ctls(struct amp_priv *amp_priv, int ctl_idx, int ctl_cnt __unused);

int amp_get_non_alsa_be_ctl_count(struct amp_priv *amp_priv);

#endif /* _AGM_MIXER_PLUGIN_H_ */
