pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agm_ls_fe.pc
EXTRA_DIST = $(pkgconfig_DATA)

h_sources  = $(top_srcdir)/inc/ipc_common.h

AM_CPPFLAGS := -I $(top_srcdir) -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/agm/\
               -I ${top_srcdir}/inc

AM_CPPFLAGS += -DDYNAMIC_LOG_ENABLED
lib_LTLIBRARIES      = libagm_ls_fe.la
libagm_ls_fe_ladir = $(libdir)
libagm_ls_fe_la_SOURCES   = src/agm_ls_fe_wrapper.cpp
libagm_ls_fe_la_CPPFLAGS := $(AM_CPPFLAGS)
libagm_ls_fe_la_CPPFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
libagm_ls_fe_la_LDFLAGS   = -lcutils -llog -ldl -shared -avoid-version -lutils -lrt
libagm_ls_fe_la_LIBADD     = -laudio_log_utils
