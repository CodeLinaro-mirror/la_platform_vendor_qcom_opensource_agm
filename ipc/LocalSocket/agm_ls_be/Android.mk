LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := agm_ls_be
LOCAL_MODULE_OWNER  := qti
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := \
    src/agm_ipc_ls_be.cpp \
    src/agm_ls_be_daemon.cpp \
    src/agm_ls_be_wrapper.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc
LOCAL_HEADER_LIBRARIES := libagm_headers

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libutils \
    libagmclient

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_DYNAMIC_LOG)), true)
LOCAL_CFLAGS           += -DDYNAMIC_LOG_ENABLED
LOCAL_C_INCLUDES       += $(TOP)/external/expat/lib/expat.h
LOCAL_SHARED_LIBRARIES += libaudio_log_utils
LOCAL_SHARED_LIBRARIES += libexpat
LOCAL_HEADER_LIBRARIES += libaudiologutils_headers
endif

include $(BUILD_EXECUTABLE)
