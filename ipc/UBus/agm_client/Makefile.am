pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmclient.pc
EXTRA_DIST = $(pkgconfig_DATA)

agmclient_sources = ./src/agm_client_wrapper_ubus.cpp

AM_CPPFLAGS = -Wno-narrowing
AM_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
AM_CPPFLAGS += $(GLIB_CFLAGS) -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h

lib_LTLIBRARIES = libagmubusclientwrapper.la
libagmubusclientwrapper_la_SOURCES = $(agmclient_sources)
libagmubusclientwrapper_la_CPPFLAGS = $(AM_CPPFLAGS)
libagmubusclientwrapper_la_LIBADD = -lglib-2.0 -laudio_log_utils -lar_osal
libagmubusclientwrapper_la_LDFLAGS =  -lubus -lpthread
