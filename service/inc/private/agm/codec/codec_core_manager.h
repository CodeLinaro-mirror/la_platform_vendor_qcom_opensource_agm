#ifndef _CDC_CODEC_CORE_MANAGER_H_
#define _CDC_CODEC_CORE_MANAGER_H_

/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/**
* \brief mapping for different formats supported by the CDC driver.
*/
typedef enum cdc_format
{
  CDC_FMT_PCM_DEFAULT = 0x1,
  CDC_FMT_PCM_S8 = 0x2,
  CDC_FMT_PCM_S16_LE = CDC_FMT_PCM_DEFAULT,
  CDC_FMT_PCM_S24_3LE = 0x3,
  CDC_FMT_PCM_S24_LE = 0x4,
  CDC_FMT_PCM_S32_LE = 0X5,
  CDC_FMT_PCM_MAX = 0x7F
} cdc_format_t;

/**
* \brief mapping for different custom payload param supported by the CDC driver.
*/
typedef enum cdc_cmd_custom_payload_param
{
  CDC_CMD_CUSTOM_PAYLOAD_LPI_PCM = 0,
  CDC_CMD_CUSTOM_PAYLOAD_BT_SLAVE
} cdc_cmd_custom_payload_param_t;

/* core manager calls */
/**
* \brief mapping for custom_payload.
*/

#pragma pack(1)

typedef struct custom_payload
{
  uint32_t size;
  uint8_t payload[0];
} custom_payload_t;

/**
* \brief mapping for media conf.
*/
typedef struct media_config
{
  uint32_t sample_rate;
  uint32_t channels;
  uint32_t bit_width;
  cdc_format_t codec_format;
} media_config_t;

/**
* \brief Structure ep_cmd_route_payload for CODEC_CMD_ENDPOINT_SET_ROUTING
*                                           CODEC_CMD_ENDPOINT_RESET_ROUTING
*
*/
typedef struct ep_cmd_route_payload
{
  uint32_t epid;  //EP ID
  uint32_t ep_payload_size;

/*   payload packing incase of 2 modules M1 with param id p1 and M2 with param id p2 is as follows
  payload = cdc_module_payload_t(M1) + cdc_module_payload_t(M2) */

  uint8_t ep_payload[0];
} ep_cmd_route_payload_t;

/**
* \brief Structure ep_cmd_custom_payload for CODEC_CMD_ENDPOINT_SET_CUSTOM_PAYLOAD
*
*/
typedef struct ep_cmd_custom_payload
{
  uint32_t epid;  //EP ID
  custom_payload_t ep_custom_payload;
} ep_cmd_custom_payload_t;

/**
* \brief Structure ep_cmd_custom_payload for CODEC_CMD_ENDPOINT_SET_CUSTOM_PAYLOAD
*
*/
typedef struct cdc_custom_payload
{
  uint32_t param_id;  //param ID
  uint8_t cdc_custom_payload[0];
} cdc_custom_payload_t;

/**
* \brief Structure ep_cmd_enable_payload for CODEC_CMD_ENDPOINT_ENABLE
*
*/
typedef struct ep_cmd_enable_payload
{
  uint32_t epid;  //EP ID
  media_config_t cdc_media_config;
} ep_cmd_enable_payload_t;

/**
* \brief Structure ep_cmd_disable_payload for CODEC_CMD_ENDPOINT_DISABLE
*
*/
typedef struct ep_cmd_disable_payload
{
  uint32_t epid;
} ep_cmd_disable_payload_t;

/**
* \brief Structure ep_cmd_response for CODEC_CMD_ENDPOINT_SET_ROUTING
*                                      CODEC_CMD_SET_CUSTOM_PAYLOAD
*                                      CODEC_CMD_ENDPOINT_ENABLE
*                                      CODEC_CMD_ENDPOINT_RESET_ROUTING
*                                      CODEC_CMD_ENDPOINT_DISABLE
*/
typedef struct ep_cmd_response
{
  uint32_t epid;  //EP ID
  // uint32_t status; // status 0 or 1
  //TODO: to get data from codec
} ep_cmd_response_t;

// /**
// * \brief Structure cdc_cmd_status for CODEC_CMD_CODEC_STATUS
// *
// */
// typedef struct cdc_cmd_status
// {
  // uint32_t status;
// } cdc_cmd_status_t;

/**
* \brief Structure For packing codec modules payload
*
*/
typedef struct cdc_module_payload{

    uint32_t                             module_id; /**< Valid ID of the module */
    uint32_t                             size;     /**< Valid Size of the module */
    uint8_t                             byte[0];   /**< Paylod of the module */
} cdc_module_payload_t, *pcdc_module_payload_t;

#pragma pack()
#endif //_CDC_CODEC_CORE_MANAGER_H_
