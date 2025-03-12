/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 *
 * Copyright (c) 2022-2025, Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 */

#include <agm/agm_api.h>
#include <errno.h>
#include <limits.h>
#include <linux/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sound/asound.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <tinyalsa/pcm_plugin.h>
#include <snd-card-def.h>
#include <tinyalsa/asoundlib.h>
#include <agm/utils.h>
#include <agm/codec/codec_interface.h>
#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_PCM_PLUGIN
#include <log_utils.h>
#endif

/* 2 words of uint32_t = 64 bits of mask */
#define PCM_MASK_SIZE (2)
#define PCM_FORMAT_BIT(x) ((uint64_t)1 << x)

struct codec_priv {
    struct agm_media_config *media_config;
    void *card_node;
    int device_id;
};

struct pcm_plugin_hw_constraints codec_constrs = {
    .access = 0,
    .format = 0,
    .bit_width = {
        .min = 16,
        .max = 32,
    },
    .channels = {
        .min = 1,
        .max = 8,
    },
    .rate = {
        .min = 8000,
        .max = 384000,
    },
    .periods = {
        .min = 1,
        .max = 8,
    },
    .period_bytes = {
        .min = 96,
        .max = 122880,
    },
};

static inline struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p,
                                                  int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline int param_is_interval(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static unsigned int param_get_int(struct snd_pcm_hw_params *p, int n)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        if (i->integer)
            return i->max;
    }
    return 0;
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static inline int param_is_mask(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int snd_mask_val(const struct snd_mask *mask)
{
    int i;
    for (i = 0; i < PCM_MASK_SIZE; i++) {
        if (mask->bits[i])
            return ffs(mask->bits[i]) + (i << 5) - 1;
    }
    return 0;
}

static unsigned int agm_format_to_bits(enum agm_media_format format)
{
    switch (format) {
    case AGM_FORMAT_PCM_S32_LE:
    case AGM_FORMAT_PCM_S24_LE:
        return 32;
    case AGM_FORMAT_PCM_S24_3LE:
        return 24;
    default:
    case AGM_FORMAT_PCM_S16_LE:
        return 16;
    };
}

static enum agm_media_format alsa_to_agm_format(int format)
{
    switch (format) {
    case SNDRV_PCM_FORMAT_S32_LE:
        return AGM_FORMAT_PCM_S32_LE;
    case SNDRV_PCM_FORMAT_S8:
        return AGM_FORMAT_PCM_S8;
    case SNDRV_PCM_FORMAT_S24_3LE:
        return AGM_FORMAT_PCM_S24_3LE;
    case SNDRV_PCM_FORMAT_S24_LE:
        return AGM_FORMAT_PCM_S24_LE;
    default:
    case SNDRV_PCM_FORMAT_S16_LE:
        return AGM_FORMAT_PCM_S16_LE;
    };
}

static enum agm_media_format param_get_mask_val(struct snd_pcm_hw_params *p,
                                        int n)
{
    if (param_is_mask(n)) {
        struct snd_mask *m = param_to_mask(p, n);
        int val = snd_mask_val(m);

        return alsa_to_agm_format(val);
    }
    return 0;
}

static unsigned int codec_frames_to_bytes(struct agm_media_config *config,
        unsigned int frames)
{
    return frames * config->channels *
        (agm_format_to_bits(config->format) >> 3);
}

static unsigned int codec_bytes_to_frames(unsigned int bytes,
        struct agm_media_config *config)
{
    unsigned int frame_bits = config->channels *
        agm_format_to_bits(config->format);

    return bytes * 8 / frame_bits;
}

static int codec_hw_params(struct pcm_plugin *plugin,
                             struct snd_pcm_hw_params *params)
{
    struct codec_priv *priv = plugin->priv;
    int ret = 0;

    AGM_LOGI("%s\n", __func__);
    priv->media_config->rate =  param_get_int(params, SNDRV_PCM_HW_PARAM_RATE);
    priv->media_config->channels = param_get_int(params, SNDRV_PCM_HW_PARAM_CHANNELS);
    priv->media_config->format = param_get_mask_val(params, SNDRV_PCM_HW_PARAM_FORMAT);
    AGM_LOGI("%s: rate %d ch %d bits %d\n", __func__,
            priv->media_config->rate, priv->media_config->channels,
            agm_format_to_bits(priv->media_config->format));
    return ret;
}

static int codec_sw_params(struct pcm_plugin *plugin,
                             struct snd_pcm_sw_params *sparams)
{
    AGM_LOGI("%s\n", __func__);
    return 0;
}

static int codec_prepare(struct pcm_plugin *plugin)
{
    struct codec_priv *priv = plugin->priv;
    struct codec_media_config config;
    int ret = 0;

    AGM_LOGI("%s\n", __func__);
    /* Q6 CODEC ENABLE ENDPOINT */

    memset(&config, 0, sizeof(config));
    config.rate = priv->media_config->rate;
    config.ch = priv->media_config->channels;
    config.bit_width = agm_format_to_bits(priv->media_config->format);
    ret = cdc_enable_endpoint(priv->device_id, &config);
    return ret;
}

static int codec_start(struct pcm_plugin *plugin)
{
    struct q6_pcm_priv *priv = plugin->priv;
    int ret = 0;

    AGM_LOGI("%s\n", __func__);
    return ret;
}

static int codec_close(struct pcm_plugin *plugin)
{
    struct codec_priv *priv = plugin->priv;
    int ret = 0;

    AGM_LOGI("%s\n", __func__);
    ret = cdc_disable_endpoint(priv->device_id);
    snd_card_def_put_card(priv->card_node);
    free(priv->media_config);
    free(plugin->priv);
    free(plugin);

    return ret;
}

static void* codec_mmap(struct pcm_plugin *plugin, void *addr __unused, size_t length, int prot __unused,
                               int flags __unused, off_t offset)
{
    return MAP_FAILED;
}

static int codec_munmap(struct pcm_plugin *plugin, void *addr, size_t length)
{
    return 0;
}

static int codec_ioctl(struct pcm_plugin *plugin, int cmd, void *args)
{
    return 0;
}

static int codec_poll(struct pcm_plugin *plugin, struct pollfd *pfd,
        nfds_t nfds __attribute__ ((unused)), int timeout)
{
    return 0;
}

static int codec_drop(struct pcm_plugin *plugin)
{
    return 0;
}

static int codec_sync_ptr(struct pcm_plugin *plugin,
                            struct snd_pcm_sync_ptr *sync_ptr)
{
    return 0;
}

static int codec_writei_frames(struct pcm_plugin *plugin, struct snd_xferi *x)
{
    return 0;
}

static int codec_readi_frames(struct pcm_plugin *plugin, struct snd_xferi *x)
{
    return 0;
}

static int codec_ttstamp(struct pcm_plugin *plugin, int *tstamp __unused)
{
    return 0;
}

struct pcm_plugin_ops codec_ops = {
    .close = codec_close,
    .hw_params = codec_hw_params,
    .sw_params = codec_sw_params,
    .sync_ptr = codec_sync_ptr,
    .writei_frames = codec_writei_frames,
    .readi_frames = codec_readi_frames,
    .ttstamp = codec_ttstamp,
    .prepare = codec_prepare,
    .start = codec_start,
    .drop = codec_drop,
    .mmap = codec_mmap,
    .munmap = codec_munmap,
    .poll = codec_poll,
    .ioctl = codec_ioctl,
};

PCM_PLUGIN_OPEN_FN(codec_pcm_plugin)
{
    struct pcm_plugin *codec_plugin;
    struct codec_priv *priv;
    struct agm_media_config *media_config;
    int ret = 0;
    void *card_node, *pcm_node;

    AGM_LOGI("%s\n", __func__);
    codec_plugin = calloc(1, sizeof(struct pcm_plugin));
    if (!codec_plugin)
        return -ENOMEM;

    priv = calloc(1, sizeof(struct codec_priv));
    if (!priv) {
        ret = -ENOMEM;
        goto err_plugin_free;
    }

    media_config = calloc(1, sizeof(struct agm_media_config));
    if (!media_config) {
        ret = -ENOMEM;
        goto err_priv_free;
    }

    card_node = snd_card_def_get_card(card);
    if (!card_node) {
        ret = -EINVAL;
        goto err_media_free;
    }

    pcm_node = snd_card_def_get_node(card_node, device, SND_NODE_TYPE_PCM);
    if (!pcm_node) {
        ret = -EINVAL;
        goto err_card_put;
    }

    codec_constrs.access = (PCM_FORMAT_BIT(SNDRV_PCM_ACCESS_RW_INTERLEAVED) |
            PCM_FORMAT_BIT(SNDRV_PCM_ACCESS_RW_NONINTERLEAVED));
    codec_constrs.format = (PCM_FORMAT_BIT(SNDRV_PCM_FORMAT_S16_LE) |
            PCM_FORMAT_BIT(SNDRV_PCM_FORMAT_S24_LE) |
            PCM_FORMAT_BIT(SNDRV_PCM_FORMAT_S24_3LE) |
            PCM_FORMAT_BIT(SNDRV_PCM_FORMAT_S32_LE));

    codec_plugin->card = card;
    codec_plugin->ops = &codec_ops;
    codec_plugin->node = pcm_node;
    codec_plugin->constraints = &codec_constrs;
    codec_plugin->mode = mode;
    codec_plugin->priv = priv;

    priv->media_config = media_config;
    priv->card_node = card_node;
    priv->device_id = device;

    *plugin = codec_plugin;

    return 0;

err_card_put:
    snd_card_def_put_card(card_node);
err_media_free:
    free(media_config);
err_priv_free:
    free(priv);
err_plugin_free:
    free(codec_plugin);
    if (ret < 0)
       return ret;
    else
       return -ret;
}
