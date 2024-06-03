LOCAL_PATH := $(call my-dir)
ifeq ($(AUDIO_FEATURE_AGM_USES_SW_BINDER), true)
include $(call all-makefiles-under, $(LOCAL_PATH)/SwBinders)
else
  ifeq ($(filter $(TARGET_BOARD_DERIVATIVE_SUFFIX), _sdv _cdcsdv),$(TARGET_BOARD_DERIVATIVE_SUFFIX))
      agm_ipc = HwBinders
      agm_ipc += Socket
      include $(call all-named-subdir-makefiles, $(agm_ipc))
  else
      include $(call all-named-subdir-makefiles, HwBinders)
  endif
endif
