#pragma once
#ifdef __H2XML__
#include "ar_osal_types.h"
#endif

/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/* Driver data module to host CODEC path configurations.
   Supported Param ID(s):
                      PARAM_ID_DRIVER_MODULE_LIST
*/
#define MODULE_ID_DRIVER_MODULE_LIST                              0x0C000040

/*------------------------------------------------------------------------*//**
    @h2xmlm_module       {"MODULE_ID_DRIVER_MODULE_LIST", MODULE_ID_DRIVER_MODULE_LIST }
    @h2xmlm_displayName  {"LIST_OF_CODEC_DRIVER_MODULES"}
    @h2xmlm_description  { List of Codec Driver Modules \n
                           Supports following params: \n
                            PARAM_ID_DRIVER_MODULE_LIST \n}
    @h2xmlp_toolPolicy  {Calibration}
    @{
----------------------------------------------------------------------------*/

/* Driver data to host CODEC path source sink connection configurations for
   module MODULE_ID_DRIVER_MODULE_LIST.
   Supported payload:
                    DRIVER_MODULE_LIST*/
#define PARAM_ID_DRIVER_MODULE_LIST        0x0C000041

/* Payload of the PARAM_ID_DRIVER_MODULE_LIST parameter
 */

#ifndef __H2XML__
#pragma pack(1)
#endif

/** @h2xmlp_subStruct */
typedef struct _DRIVER_MODULE_ID {
    uint32_t          Module_ID;          /* Module ID*/
    /**< @h2xmle_description { Module ID of driver}
         @h2xmle_rangeList   {"NONE"=0x00000;
                              "Bolero "=0x0C000020;
                              "SDCA "=0x0C000030}
         @h2xmle_default         {0}
     */
} DRIVER_MODULE_ID;

/** @h2xmlp_parameter   {"PARAM_ID_DRIVER_MODULE_LIST",
                           PARAM_ID_DRIVER_MODULE_LIST}
    @h2xmlp_persistType {None}
    @h2xmlp_description {List of driver modules.}
*/
typedef struct _DRIVER_MODULE_LIST {
    uint32_t                      NumberOfModules; /* Number of driver modules.*/
/**< @h2xmle_description { Number of driver modules.}
       @h2xmle_default     {0}
*/
    DRIVER_MODULE_ID    ModuleList[0];       /* driver module details.*/
/**< @h2xmle_description { Variable size array of type DRIVER_MODULE_LIST
                              to provide driver module details.}
        @h2xmle_variableArraySize {"NumberOfModules"}
        @h2xmle_default     {0}
*/
}DRIVER_MODULE_LIST;

#ifndef __H2XML__
#pragma pack()
#endif

/** @} */           /* End of Module */









