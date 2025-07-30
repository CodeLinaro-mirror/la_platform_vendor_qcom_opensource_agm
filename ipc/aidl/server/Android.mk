LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE        := agmipcservice
LOCAL_MODULE_OWNER  := qti
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_C_INCLUDES    := $(LOCAL_PATH)/inc

LOCAL_CLANG             := true
LOCAL_TIDY              := true
LOCAL_CFLAGS            += -v -Wall -Wthread-safety

LOCAL_VINTF_FRAGMENTS += agmipcservice.xml

LOCAL_SRC_FILES     :=  \
    Service.cpp \
    AgmServerWrapper.cpp

LOCAL_STATIC_LIBRARIES := libagmaidltypeconverter libaidlcommonsupport

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libbinder_ndk \
    libbase \
    libcutils \
    libutils \
    libagm \
    vendor.qti.hardware.agm-V1-ndk

ifneq ($(filter $(TARGET_BOARD_DERIVATIVE_SUFFIX), _sdv _cdcsdv),)
LOCAL_CFLAGS += -DSOCKET_ENABLED
LOCAL_SHARED_LIBRARIES += libagmsocket_server
LOCAL_HEADER_LIBRARIES += libagmsocket_server_headers
endif

ifeq ($(ENABLE_HYP), true)
LOCAL_SHARED_LIBRARIES += libar-gsl_fe
LOCAL_HEADER_LIBRARIES += libar-gsl_fe_headers
else
LOCAL_SHARED_LIBRARIES += libar-gsl
endif

include $(BUILD_EXECUTABLE)

ifeq ($(TARGET_GVMGH_SPECIFIC), false)
include $(CLEAR_VARS)
LOCAL_MODULE       := init.qti.AGMIPC.sh
LOCAL_MODULE_TAGS  := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := $(LOCAL_MODULE)
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR_EXECUTABLES)
include $(BUILD_PREBUILT)
endif
