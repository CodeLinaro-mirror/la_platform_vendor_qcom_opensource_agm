LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libagmclient
LOCAL_MODULE_OWNER := qti
LOCAL_VENDOR_MODULE := true

ifeq ($(BOARD_SUPPORTS_RAMDISK_EARLY_INIT), true)
# LOCAL_CFLAGS += -DAR_EARLY_CHIME
endif

LOCAL_SRC_FILES := \
    src/agm_client_wrapper.cpp\
    src/AGMCallback.cpp

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libutils \
    liblog \
    libcutils \
    libhardware \
    libbase \
    vendor.qti.hardware.AGMIPC@1.0 \
    libagmsocket_client

LOCAL_HEADER_LIBRARIES := libagm_headers \
    libagmsocket_client_headers

include $(BUILD_SHARED_LIBRARY)
