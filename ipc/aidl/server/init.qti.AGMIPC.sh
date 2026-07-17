#! /vendor/bin/sh
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries
# SPDX-License-Identifier: BSD-3-Clause-Clear

audio_arch=`getprop ro.boot.audio`
if [ "$audio_arch" == "audioreach" ]; then
    enable vendor.agm-1-0
    start vendor.agm-1-0
fi
