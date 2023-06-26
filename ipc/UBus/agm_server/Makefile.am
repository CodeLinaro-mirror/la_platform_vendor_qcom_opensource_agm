pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmserver.pc
EXTRA_DIST = $(pkgconfig_DATA)

h_sources = ./inc/agm-ubus-utils.h \
            ./inc/agm_server_wrapper_ubus.h

AM_CPPFLAGS = -Wno-narrowing
AM_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
AM_CPPFLAGS += -I ${WORKSPACE}/vendor/qcom/opensource/agm/service/inc
AM_CPPFLAGS += $(GLIB_CFLAGS) -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h

library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)/agm

agmserver_sources = ./src/agm_server_wrapper_ubus.cpp \
                    ./src/agm-ubus-utils.cpp

lib_LTLIBRARIES = libagmubusserverwrapper.la
libagmubusserverwrapper_la_SOURCES = $(agmserver_sources)
libagmubusserverwrapper_la_CPPFLAGS = $(AM_CPPFLAGS)
libagmubusserverwrapper_la_LIBADD = -lglib-2.0 -lagm -laudio_log_utils -lar_osal
libagmubusserverwrapper_la_LDFLAGS =  -lubus -lpthread

bin_PROGRAMS := agm_server

agm_server_SOURCES := ./src/agm-server-daemon.cpp
agm_server_la_CPPFLAGS := $(AM_CPPFLAGS)
agm_server_LDADD = libagmubusserverwrapper.la -lglib-2.0 -lagm -laudio_log_utils -lar_osal
agm_server_la_LDFLAGS = -ldl -shared -avoid-version -lpthread -lubus
