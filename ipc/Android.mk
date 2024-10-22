LOCAL_PATH := $(call my-dir)
ifeq ($(AUDIO_FEATURE_AGM_USES_SW_BINDER), true)
include $(call all-makefiles-under, $(LOCAL_PATH)/SwBinders)
else
agm_ipc = HwBinders
agm_ipc += Socket
include $(call all-named-subdir-makefiles, $(agm_ipc))
endif
