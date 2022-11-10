pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agm.pc
EXTRA_DIST = $(pkgconfig_DATA)

h_sources = ${top_srcdir}/inc/public/agm/agm_api.h \
            ${top_srcdir}/inc/public/agm/agm_list.h \
            ${top_srcdir}/inc/public/agm/utils.h \
            ${top_srcdir}/inc/private/agm/metadata.h \
            ${top_srcdir}/inc/private/agm/graph.h \
            ${top_srcdir}/inc/private/agm/session_obj.h \
            ${top_srcdir}/inc/private/agm/device.h

AM_CFLAGS = @SPF_CFLAGS@
AM_CFLAGS += -I ${top_srcdir}/inc/public
AM_CFLAGS += -I ${top_srcdir}/inc/private

library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)/agm

agm_sources = ${top_srcdir}/src/graph.c \
              ${top_srcdir}/src/graph_module.c \
              ${top_srcdir}/src/device.c \
              ${top_srcdir}/src/device_hw_ep.c \
              ${top_srcdir}/src/metadata.c \
              ${top_srcdir}/src/session_obj.c \
              ${top_srcdir}/src/agm.c \
              ${top_srcdir}/src/utils.c

lib_LTLIBRARIES = libagm.la
libagm_la_SOURCES = $(agm_sources)
if !TARGET_HYPERVISOR
libagm_la_LIBADD = -lasound -lar_osal -lar_gsl -lats
else
libagm_la_LIBADD = -lasound -lar_osal -lats -lgsl_fe
endif
AM_CFLAGS += -DDEVICE_USES_ALSALIB

libagm_la_CFLAGS := $(AM_CFLAGS) -DACDB_DELTA_FILE_PATH=\"/data/audio/delta\"
libagm_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -fcommon
libagm_la_CFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
libagm_la_LDFLAGS = -module -shared -avoid-version

if VENDOR_PARTITION
    libagm_la_CFLAGS += -DACDB_PATH=\"/vendor/etc/acdbdata\"
else
    libagm_la_CFLAGS += -DACDB_PATH=\"/etc/acdbdata\"
endif
