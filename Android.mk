ifeq ($(TARGET_BOARD_PLATFORM), monaco)
ifneq ($(AUDIO_USE_STUB_HAL), true)
    include $(call all-subdir-makefiles)
endif
endif
