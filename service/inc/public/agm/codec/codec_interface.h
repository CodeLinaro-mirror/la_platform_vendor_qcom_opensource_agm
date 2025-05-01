/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _CODEC_INTERFACE_H_
#define _CODEC_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <agm/agm_list.h>

struct codec_module_payload {
    uint32_t mid;
    size_t size;
    uint8_t *data;
};

struct codec_module_payload_list {
    uint32_t total_payload_size;
    uint32_t num_mp;
    struct codec_module_payload *mp_arr;
};
typedef enum codec_media_format
{
    CDC_MEDIA_FMT_PCM_INVALID = 0x0,
    CDC_MEDIA_FMT_PCM_S8 = 0x1,
    CDC_MEDIA_FMT_PCM_S16_LE = 0x2,
    CDC_MEDIA_FMT_PCM_S24_3LE = 0x3,
    CDC_MEDIA_FMT_PCM_S24_LE = 0x4,
    CDC_MEDIA_FMT_PCM_S32_LE = 0x5,
    CDC_MEDIA_FMT_PCM_MAX
} codec_media_format_t;

struct codec_media_config {
    int32_t rate;
    int32_t ch;
    int32_t bit_width;
    codec_media_format_t format;
};

/**
 *
 *  \brief initializes codec interface
 *
 *  \return 0 on success, error code on failure.
 */
int cdc_interface_init();

/**
 *  \brief de-initializes codec interface
 *
 *  \return 0 on success, error code on failure.
 */
int cdc_interface_deinit();

/**
 * \brief Enables routing of codec endpoint
 *
 * \param[in] ep_id - endpoint id
 * \param[in] mp_list - list of routing payload for end point
 * \param[in] set - true to set, false otherwise
 *
 *  \return 0 on success, error code on failure.
 */
int cdc_route_endpoint(int ep_id, struct codec_module_payload_list *mp_list, bool set);

/**
 * \brief Enables codec endpoint
 *
 * \param[in] ep_id - end point id
 * \param[in] config - media config for codec endpoint
 *
 *  \return 0 on success, error code on failure.
 */
int cdc_enable_endpoint(int ep_id, struct codec_media_config *config);

/**
 * \brief Set custom payload to codec endpoint
 *
 * \param[in] param_id - param id supported by codec core
 * \param[in] ep_payload - custom payload
 * \param[in] payload_size - size of custom payload
 *
 *  \return 0 on success, error code on failure.
 */
int cdc_set_custom_payload_endpoint(int param_id, void *ep_payload, int payload_size);

/**
 * \brief Disables codec endpoint
 *
 * \param[in] ep_id - end point id
 *
 *  \return 0 on success, error code on failure.
 */
int cdc_disable_endpoint(int ep_id);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif //_CODEC_INTERFACE_H_
