pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agm_ls_fe.pc
EXTRA_DIST = $(pkgconfig_DATA)

AUTOMAKE_OPTIONS = subdir-objects

h_sources  = $(srcdir)/inc/ipc_common.h

AM_CPPFLAGS = -I $(top_srcdir) -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/audio_log_utils/
AM_CPPFLAGS += -I ${top_srcdir}/inc
AM_CPPFLAGS += -I ${top_srcdir}/service/inc/public
AM_CPPFLAGS += -I ${srcdir}/inc

lib_LTLIBRARIES      = libagm_ls_fe.la
libagm_ls_fe_ladir = $(libdir)
libagm_ls_fe_la_SOURCES   = src/agm_ls_fe_wrapper.cpp
libagm_ls_fe_la_CPPFLAGS = $(AM_CPPFLAGS)
libagm_ls_fe_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat

libagm_ls_fe_la_LDFLAGS   = -lcutils -llog -ldl -shared -avoid-version -lutils -lrt
