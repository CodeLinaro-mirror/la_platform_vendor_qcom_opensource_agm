/**
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

@VintfStability
@Backing(type="int")
enum CodecMediaFormat {
        CDC_MEDIA_FMT_PCM_INVALID,
        CDC_MEDIA_FMT_PCM_S8,          /**< 8-bit signed */
        CDC_MEDIA_FMT_PCM_S16_LE,      /**< 16-bit signed */
        CDC_MEDIA_FMT_PCM_S24_3LE,     /**< 24-bits in 3-bytes */  
        CDC_MEDIA_FMT_PCM_S24_LE,      /**< 24-bits in 4-bytes */
        CDC_MEDIA_FMT_PCM_S32_LE,      /**< 32-bit signed */
        CDC_MEDIA_FMT_PCM_MAX,
}
