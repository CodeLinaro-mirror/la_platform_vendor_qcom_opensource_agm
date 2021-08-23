ifeq ($(call is-board-platform-in-list, sdm845 msmnile kona lahaina sm6150),true)


include $(call all-subdir-makefiles)


endif # is-board-platform-in-list
