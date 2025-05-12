LOCAL_PATH := $(call my-dir)

#temporary solution for VTS test vts_treble_vintf_vendor_test failure
#Adding Android U and target check to avoid AMS service inclusion for elite
#on LA3.6.0
ifneq ( ,$(filter U UpsideDownCake 14 V VanillaIceCream 15, $(PLATFORM_VERSION)))
ifeq (,$(filter $(PRODUCT_NAME), msmnile_au sm6150_au))
# Build libagmsocket_headers
include $(CLEAR_VARS)
LOCAL_MODULE                := libagmsocket_server_headers
LOCAL_VENDOR_MODULE         := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc
include $(BUILD_HEADER_LIBRARY)

# Build socket library
include $(CLEAR_VARS)
LOCAL_MODULE               := libagmsocket_server
LOCAL_MODULE_OWNER         := qti
LOCAL_VENDOR_MODULE        := true
LOCAL_MODULE_TAGS          := optional

LOCAL_CFLAGS               := -v -Wall
LOCAL_C_INCLUDES           := $(LOCAL_PATH)/inc/
LOCAL_SRC_FILES            := src/agm_conn_server.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libutils \
    libagm \
    libagmsocket

LOCAL_HEADER_LIBRARIES := libarosal_headers

LOCAL_HEADER_LIBRARIES += libagmsocket_headers \
        libagmsocket_server_headers \
        libagm_headers

ifeq ($(ENABLE_HYP), true)
LOCAL_SHARED_LIBRARIES += libar-gsl_fe
LOCAL_HEADER_LIBRARIES += libar-gsl_fe_headers
else
LOCAL_SHARED_LIBRARIES += libar-gsl
LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := libar-gsl
LOCAL_CFLAGS += -DAGM_HW_RSC_CFG_EN
endif

include $(BUILD_SHARED_LIBRARY)
endif
#other than Android U
else
include $(CLEAR_VARS)
LOCAL_MODULE                := libagmsocket_server_headers
LOCAL_VENDOR_MODULE         := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc
include $(BUILD_HEADER_LIBRARY)

# Build socket library
include $(CLEAR_VARS)
LOCAL_MODULE               := libagmsocket_server
LOCAL_MODULE_OWNER         := qti
LOCAL_VENDOR_MODULE        := true
LOCAL_MODULE_TAGS          := optional

LOCAL_CFLAGS               := -v -Wall
LOCAL_C_INCLUDES           := $(LOCAL_PATH)/inc/
LOCAL_SRC_FILES            := src/agm_conn_server.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libutils \
    libagm \
    libagmsocket

LOCAL_HEADER_LIBRARIES := libarosal_headers

LOCAL_HEADER_LIBRARIES += libagmsocket_headers \
        libagmsocket_server_headers \
        libagm_headers

ifeq ($(ENABLE_HYP), true)
LOCAL_SHARED_LIBRARIES += libar-gsl_fe
LOCAL_HEADER_LIBRARIES += libar-gsl_fe_headers
else
LOCAL_SHARED_LIBRARIES += libar-gsl
LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := libar-gsl
LOCAL_CFLAGS += -DAGM_HW_RSC_CFG_EN
endif

include $(BUILD_SHARED_LIBRARY)
endif
