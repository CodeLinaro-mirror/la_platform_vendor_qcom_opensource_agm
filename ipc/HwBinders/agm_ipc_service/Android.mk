LOCAL_PATH := $(call my-dir)

#temporary solution for VTS test vts_treble_vintf_vendor_test failure
#Adding Android U and target check to avoid AMS service inclusion for elite
#on LA3.6.0
ifneq ( ,$(filter U UpsideDownCake 14 V VanillaIceCream 15, $(PLATFORM_VERSION)))
ifeq (,$(filter $(PRODUCT_NAME), msmnile_au sm6150_au))
include $(CLEAR_VARS)

LOCAL_MODULE        := vendor.qti.hardware.AGMIPC@1.0-impl
LOCAL_MODULE_OWNER  := qti
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -v -Wall
LOCAL_C_INCLUDES    := $(TOP)/vendor/qcom/opensource/agm/ipc/HwBinders/agm_ipc_client/
LOCAL_SRC_FILES     := src/agm_server_wrapper.cpp

LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := libar-gsl
LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libutils \
    liblog \
    libcutils \
    libhardware \
    libbase \
    vendor.qti.hardware.AGMIPC@1.0 \
    libagm

LOCAL_HEADER_LIBRARIES := libarosal_headers

ifeq ($(ENABLE_HYP), true)
LOCAL_SHARED_LIBRARIES += libar-gsl_fe
LOCAL_HEADER_LIBRARIES += libar-gsl_fe_headers
else
LOCAL_SHARED_LIBRARIES += libar-gsl
LOCAL_CFLAGS += -DAGM_HW_RSC_CFG_EN
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE               := vendor.qti.hardware.AGMIPC@1.0-service
LOCAL_INIT_RC              := vendor.qti.hardware.AGMIPC@1.0-service.rc
LOCAL_VENDOR_MODULE        := true

ifeq ($(TARGET_GVMGH_SPECIFIC), false)
ifneq ($(filter $(TARGET_BOARD_DERIVATIVE_SUFFIX), _cdcsdv),)
LOCAL_INIT_RC := vendor.qti.hardware.AGMIPC@1.0-service-v2.rc
else
LOCAL_INIT_RC := vendor.qti.hardware.AGMIPC@1.0-service-v2.rc
LOCAL_VINTF_FRAGMENTS := vendor.qti.hardware.AGMIPC@1.0-service-v2.xml
endif
else
LOCAL_INIT_RC := vendor.qti.hardware.AGMIPC@1.0-service.rc
endif
ifeq ($(TARGET_BOARD_PLATFORM)$(TARGET_BOARD_SUFFIX), msmnile_au)
LOCAL_CFLAGS  += -DPLATFORM_MSMNILE_AU
endif
ifeq ($(BOARD_SUPPORTS_RAMDISK_EARLY_INIT), true)
# LOCAL_CFLAGS  += -DAR_EARLY_CHIME
endif
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_OWNER         := qti

LOCAL_C_INCLUDES           := $(TOP)/vendor/qcom/opensource/agm/ipc/HwBinders/agm_ipc_client/
LOCAL_SRC_FILES            := src/service.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbase \
    libutils \
    libhardware \
    libhidlbase \
    vendor.qti.hardware.AGMIPC@1.0 \
    vendor.qti.hardware.AGMIPC@1.0-impl \
    libagm \
    libselinux \
    libagmsocket_server

LOCAL_HEADER_LIBRARIES := libagmsocket_server_headers

ifneq ($(filter $(TARGET_BOARD_DERIVATIVE_SUFFIX), _sdv _cdcsdv),)
LOCAL_CFLAGS  += -DAR_EARLY_CHIME
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
endif
#other than Android U
else
include $(CLEAR_VARS)

LOCAL_MODULE        := vendor.qti.hardware.AGMIPC@1.0-impl
LOCAL_MODULE_OWNER  := qti
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS        += -v -Wall
LOCAL_C_INCLUDES    := $(TOP)/vendor/qcom/opensource/agm/ipc/HwBinders/agm_ipc_client/
LOCAL_SRC_FILES     := src/agm_server_wrapper.cpp

LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := libar-gsl
LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libutils \
    liblog \
    libcutils \
    libhardware \
    libbase \
    vendor.qti.hardware.AGMIPC@1.0 \
    libagm

LOCAL_HEADER_LIBRARIES := libarosal_headers

ifeq ($(ENABLE_HYP), true)
LOCAL_SHARED_LIBRARIES += libar-gsl_fe
LOCAL_HEADER_LIBRARIES += libar-gsl_fe_headers
else
LOCAL_SHARED_LIBRARIES += libar-gsl
LOCAL_CFLAGS += -DAGM_HW_RSC_CFG_EN
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE               := vendor.qti.hardware.AGMIPC@1.0-service
LOCAL_INIT_RC              := vendor.qti.hardware.AGMIPC@1.0-service.rc
LOCAL_VENDOR_MODULE        := true

ifeq ($(TARGET_GVMGH_SPECIFIC), false)
LOCAL_INIT_RC := vendor.qti.hardware.AGMIPC@1.0-service-v2.rc
LOCAL_VINTF_FRAGMENTS := vendor.qti.hardware.AGMIPC@1.0-service-v2.xml
else
LOCAL_INIT_RC := vendor.qti.hardware.AGMIPC@1.0-service.rc
endif
ifeq ($(TARGET_BOARD_PLATFORM)$(TARGET_BOARD_SUFFIX), msmnile_au)
LOCAL_CFLAGS  += -DPLATFORM_MSMNILE_AU
endif
ifeq ($(BOARD_SUPPORTS_RAMDISK_EARLY_INIT), true)
# LOCAL_CFLAGS  += -DAR_EARLY_CHIME
endif
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_OWNER         := qti

LOCAL_C_INCLUDES           := $(TOP)/vendor/qcom/opensource/agm/ipc/HwBinders/agm_ipc_client/
LOCAL_SRC_FILES            := src/service.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbase \
    libutils \
    libhardware \
    libhidlbase \
    vendor.qti.hardware.AGMIPC@1.0 \
    vendor.qti.hardware.AGMIPC@1.0-impl \
    libagm \
    libselinux \
    libagmsocket_server

LOCAL_HEADER_LIBRARIES := libagmsocket_server_headers

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
endif
