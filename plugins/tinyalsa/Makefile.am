pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmplugin.pc
EXTRA_DIST = $(pkgconfig_DATA)

library_includedir = $(includedir)

AM_CFLAGS := -Wno-unused-parameter
if !BUILDSYSTEM_OPENWRT
AM_CFLAGS += -I $(top_srcdir)/include -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/agm/
AM_CFLAGS += -I ${STAGING_KERNEL_DIR}/include/uapi
AM_CFLAGS += -I $(top_srcdir)/include -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/agm/ -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include
AM_CFLAGS += $(GLIB_CFLAGS) -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
AM_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED
AM_CFLAGS += -DSND_AUDIOCODEC_WMA_PRO=0x00000017 -DSND_AUDIOCODEC_ALAC=0x0000000F -DSND_AUDIOCODEC_APE=0x00000010
AM_CFLAGS += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/acdbdata/
AM_CFLAGS += -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/sndparser/
AM_CFLAGS += -Wno-unused-parameter
AM_CFLAGS += -DDYNAMIC_LOG_ENABLED
endif

lib_LTLIBRARIES      = libagm_pcm_plugin.la
libagm_pcm_plugin_la_SOURCES   = src/agm_pcm_plugin.c
libagm_pcm_plugin_la_CFLAGS := $(AM_CFLAGS)
libagm_pcm_plugin_la_LDFLAGS  = -ltinyalsa -lsndcardparser -avoid-version -shared
if BUILDSYSTEM_OPENWRT
libagm_pcm_plugin_la_LDFLAGS += -lagm
else
libagm_pcm_plugin_la_LDFLAGS += -laudio_log_utils
endif

lib_LTLIBRARIES      += libagm_pcm_passthrough_plugin.la
libagm_pcm_passthrough_plugin_la_SOURCES   = src/agm_pcm_plugin.c
libagm_pcm_passthrough_plugin_la_CFLAGS := $(AM_CFLAGS) -DBYPASS_AGM_IPC
libagm_pcm_passthrough_plugin_la_LDFLAGS  = -ltinyalsa -lsndcardparser -avoid-version -shared
if BUILDSYSTEM_OPENWRT
libagm_pcm_passthrough_plugin_la_LDFLAGS += -lagm
else
libagm_pcm_passthrough_plugin_la_LDFLAGS += -laudio_log_utils
endif


if !BUILDSYSTEM_OPENWRT
lib_LTLIBRARIES      += libagm_compress_plugin.la
libagm_compress_plugin_la_SOURCES   = src/agm_compress_plugin.c
libagm_compress_plugin_la_CFLAGS := $(AM_CFLAGS)
libagm_compress_plugin_la_LDFLAGS   = -ltinyalsa -ltinycompress -lsndcardparser -lpthread  -avoid-version -laudio_log_utils -shared

lib_LTLIBRARIES      += libagm_compress_passthrough_plugin.la
libagm_compress_passthrough_plugin_la_SOURCES   = src/agm_compress_plugin.c
libagm_compress_passthrough_plugin_la_CFLAGS := $(AM_CFLAGS) -DBYPASS_AGM_IPC
libagm_compress_passthrough_plugin_la_LDFLAGS   = -ltinyalsa -ltinycompress -lsndcardparser -lpthread  -avoid-version -laudio_log_utils -shared
endif

lib_LTLIBRARIES      += libagm_mixer_plugin.la
libagm_mixer_plugin_la_SOURCES   = src/agm_mixer_plugin.c
libagm_mixer_plugin_la_CFLAGS := $(AM_CFLAGS)
libagm_mixer_plugin_la_LDFLAGS  = -ltinyalsa -lsndcardparser -avoid-version -shared
if BUILDSYSTEM_OPENWRT
libagm_mixer_plugin_la_LDFLAGS += -lagm
else
libagm_mixer_plugin_la_LDFLAGS += -laudio_log_utils
endif

lib_LTLIBRARIES      += libagm_mixer_passthrough_plugin.la
libagm_mixer_passthrough_plugin_la_SOURCES   = src/agm_mixer_plugin.c
libagm_mixer_passthrough_plugin_la_CFLAGS := $(AM_CFLAGS) -DBYPASS_AGM_IPC
libagm_mixer_passthrough_plugin_la_LDFLAGS  = -ltinyalsa -lsndcardparser -avoid-version -shared
if BUILDSYSTEM_OPENWRT
libagm_mixer_passthrough_plugin_la_LDFLAGS += -lagm
else
libagm_mixer_passthrough_plugin_la_LDFLAGS += -laudio_log_utils
endif
