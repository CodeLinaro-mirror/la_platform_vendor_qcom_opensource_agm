LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH := $(call my-dir)
ifeq ($(AUDIO_FEATURE_AGM_USES_SW_BINDER), true)
include $(call all-makefiles-under, $(LOCAL_PATH)/SwBinders)
else
ifneq ($(filter $(TARGET_BOARD_DERIVATIVE_SUFFIX), _sdv _cdcsdv),)
agm_ipc = aidl
agm_ipc += Socket
include $(call all-named-subdir-makefiles, $(agm_ipc))
else
include $(MY_LOCAL_PATH)/aidl/Android.mk
endif
endif
