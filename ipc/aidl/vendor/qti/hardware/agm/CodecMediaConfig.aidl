/**
* Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.CodecMediaFormat;

/**
 * Media Config
 */
@VintfStability
parcelable CodecMediaConfig {
    int rate;
    int channels;
	int bitWidth;
    CodecMediaFormat format;
}
