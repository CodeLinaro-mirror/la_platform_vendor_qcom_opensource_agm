
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CDC_INTERFACE_H__
#define __CDC_INTERFACE_H__

#define CODEC_CMD_INVALID                               0x1000
#define CODEC_CMD_CODEC_STATUS                          0x1001
#define CODEC_CMD_ENDPOINT_SET_ROUTING                  0x1002
#define CODEC_CMD_SET_CUSTOM_PAYLOAD                    0x1003
#define CODEC_CMD_ENDPOINT_ENABLE                       0x1004
#define CODEC_CMD_ENDPOINT_RESET_ROUTING                0x1005
#define CODEC_CMD_ENDPOINT_DISABLE                      0x1006
#define CODEC_CMD_READ_REGISTER                         0x1007
#define CODEC_CMD_READ_BULK_REGISTER                    0x1008
#define CODEC_CMD_WRITE_REGISTER                        0x1009
#define CODEC_CMD_WRITE_BULK_REGISTER                   0x100A


/**
* \brief Structure cdc_command
* \brief Structure will be allocated and should be freed by IPC manager
*/
#pragma pack(1)
typedef struct cdc_command {
    uint32_t cmd_opcode;
    uint32_t token;
    uint8_t cmd_payload[0];
} cdc_command_t;

/**
* \brief Structure cdc_response
* \brief Structure Alloc will be done by codec interface
*
*/
typedef struct cdc_response {
    uint32_t response_opcode;
    int32_t status;
    uint32_t token;
    uint32_t resp_size;
    uint8_t resp_data[0];
} cdc_response_t;

#pragma pack()

#endif  //__CDC_INTERFACE_H__
