ifneq ($(TARGET_SDV_ENABLED), true)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libagmclient
LOCAL_MODULE_OWNER := qti
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -v -Wall -Wthread-safety
LOCAL_TIDY := true

LOCAL_SRC_FILES := \
    AgmClientWrapper.cpp \
    AgmCallback.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libbase \
    libcutils \
    libutils \
    vendor.qti.hardware.agm-V1-ndk \
    libbinder_ndk

ifeq ($(strip $(TARGET_USES_DLOG)),true)
LOCAL_HEADER_LIBRARIES += dlog_headers
LOCAL_SHARED_LIBRARIES += libdlog
endif

LOCAL_STATIC_LIBRARIES := libagmaidltypeconverter libaidlcommonsupport

LOCAL_HEADER_LIBRARIES := libagm_headers

include $(BUILD_SHARED_LIBRARY)
endif
