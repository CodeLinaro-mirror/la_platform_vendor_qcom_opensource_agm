/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

package vendor.qti.hardware.agm;

import vendor.qti.hardware.agm.Direction;

@VintfStability
parcelable NonAlsaAifInfo {
    // AIF name
    String aifName;
    // direction Rx or Tx
    Direction direction;
    // card id
    int card;
    // pcm id
    int pcm;
}
