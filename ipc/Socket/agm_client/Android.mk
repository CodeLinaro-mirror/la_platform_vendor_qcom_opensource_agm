LOCAL_PATH := $(call my-dir)
# Build libagmsocket_headers
include $(CLEAR_VARS)
LOCAL_MODULE                := libagmsocket_client_headers
LOCAL_VENDOR_MODULE         := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc
include $(BUILD_HEADER_LIBRARY)

# Build socket library
include $(CLEAR_VARS)
LOCAL_MODULE               := libagmsocket_client
LOCAL_MODULE_OWNER         := qti
LOCAL_VENDOR_MODULE        := true
LOCAL_MODULE_TAGS          := optional

LOCAL_CFLAGS               := -v -Wall
LOCAL_C_INCLUDES           := $(LOCAL_PATH)/inc
LOCAL_SRC_FILES            := src/agm_conn_client.cpp

LOCAL_HEADER_LIBRARIES     := libagmsocket_headers \
        libagmsocket_client_headers \
        libagm_headers

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libutils \
    libagmsocket

include $(BUILD_SHARED_LIBRARY)
