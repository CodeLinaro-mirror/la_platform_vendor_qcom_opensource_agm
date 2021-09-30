pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmtest.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CFLAGS := -Wno-unused-parameter -Wno-unused-result
if !BUILDSYSTEM_OPENWRT
AM_CFLAGS += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/
endif
AM_CFLAGS += $(GLIB_CFLAGS) -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
AM_CFLAGS += -DBACKEND_CONF_FILE=\"/etc/backend_conf.xml\"
AM_CFLAGS += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/acdbdata

lib_LTLIBRARIES      = libagmmixer.la
libagmmixer_la_SOURCES   = agmmixer.c
libagmmixer_la_CFLAGS := $(AM_CFLAGS)
libagmmixer_la_LDFLAGS   = -ltinyalsa -avoid-version
bin_PROGRAMS := agmplay
agmplay_SOURCES  := agmplay.c

agmplay_la_CFLAGS := $(AM_CFLAGS)
agmplay_LDADD    := -ltinyalsa libagmmixer.la
agmplay_LDADD    += -lexpat  -ldl

bin_PROGRAMS += agmcap
agmcap_SOURCES  := agmcap.c

agmcap_la_CFLAGS := $(AM_CFLAGS)
agmcap_LDADD    := -ltinyalsa libagmmixer.la
agmcap_LDADD    += -lexpat -ldl

bin_PROGRAMS += agmhostless
agmhostless_SOURCES  := agmhostless.c

agmhostless_la_CFLAGS := $(AM_CFLAGS)
agmhostless_LDADD    := -ltinyalsa libagmmixer.la
agmhostless_LDADD    += -lexpat -ldl

if !BUILDSYSTEM_OPENWRT
bin_PROGRAMS += agmcompressplay
agmcompressplay_SOURCES  := agmcompressplay.c

agmcompressplay_la_CFLAGS := $(AM_CFLAGS)
agmcompressplay_LDADD    := -ltinycompress -ltinyalsa libagmmixer.la
agmcompressplay_LDADD    += -lexpat -ldl

bin_PROGRAMS += agmcompresscap
agmcompresscap_SOURCES  := agmcompresscap.c

agmcompresscap_la_CFLAGS := $(AM_CFLAGS)
agmcompresscap_LDADD    := -ltinycompress -ltinyalsa libagmmixer.la
agmcompresscap_LDADD    += -lexpat -ldl
endif

bin_PROGRAMS += agmvoiceui
agmvoiceui_SOURCES  := agm_voiceui.c

agmvoiceui_la_CFLAGS := $(AM_CFLAGS)
agmvoiceui_LDADD    := -lpthread -ltinyalsa libagmmixer.la
agmvoiceui_LDADD    += -lexpat -ldl
