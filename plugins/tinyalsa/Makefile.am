pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmtinyalsaplugin.pc
EXTRA_DIST = $(pkgconfig_DATA)

library_includedir = $(includedir)

AM_CFLAGS := -Wno-unused-parameter
AM_CFLAGS += -I $(top_srcdir)/include @AGM_CFLAGS@
AM_CFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
AM_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED
AM_CFLAGS += @SNDPARSER_CFLAGS@
AM_CFLAGS += -Wno-unused-parameter
AM_CFLAGS += -DDYNAMIC_LOG_ENABLED

lib_LTLIBRARIES      = libagm_pcm_plugin.la
libagm_pcm_plugin_la_SOURCES   = src/agm_pcm_plugin.c
libagm_pcm_plugin_la_CFLAGS := $(AM_CFLAGS)
libagm_pcm_plugin_la_LDFLAGS  = -ltinyalsa -lsndcardparser -avoid-version -shared
libagm_pcm_plugin_la_LDFLAGS += -lagmclient -laudio_log_utils

lib_LTLIBRARIES      += libagm_pcm_passthrough_plugin.la
libagm_pcm_passthrough_plugin_la_SOURCES   = src/agm_pcm_plugin.c
libagm_pcm_passthrough_plugin_la_CFLAGS := $(AM_CFLAGS)
libagm_pcm_passthrough_plugin_la_LDFLAGS  = -ltinyalsa -lsndcardparser -avoid-version -shared
libagm_pcm_passthrough_plugin_la_LDFLAGS += -lagmclient -laudio_log_utils

lib_LTLIBRARIES      += libagm_compress_plugin.la
libagm_compress_plugin_la_SOURCES   = src/agm_compress_plugin.c
libagm_compress_plugin_la_CFLAGS := $(AM_CFLAGS)
libagm_compress_plugin_la_LDFLAGS   = -ltinyalsa -ltinycompress -lsndcardparser -lpthread  -avoid-version -laudio_log_utils -shared -lagmclient

lib_LTLIBRARIES      += libagm_compress_passthrough_plugin.la
libagm_compress_passthrough_plugin_la_SOURCES   = src/agm_compress_plugin.c
libagm_compress_passthrough_plugin_la_CFLAGS := $(AM_CFLAGS)
libagm_compress_passthrough_plugin_la_LDFLAGS   = -ltinyalsa -lsndcardparser -lpthread  -avoid-version -laudio_log_utils -shared -lagmclient

lib_LTLIBRARIES      += libagm_mixer_plugin.la
libagm_mixer_plugin_la_SOURCES   = src/agm_mixer_plugin.c
libagm_mixer_plugin_la_CFLAGS := $(AM_CFLAGS)
libagm_mixer_plugin_la_LDFLAGS  = -ltinyalsa -lsndcardparser -avoid-version -shared
libagm_mixer_plugin_la_LDFLAGS += -lagmclient -laudio_log_utils

lib_LTLIBRARIES      += libagm_mixer_passthrough_plugin.la
libagm_mixer_passthrough_plugin_la_SOURCES   = src/agm_mixer_plugin.c
libagm_mixer_passthrough_plugin_la_CFLAGS := $(AM_CFLAGS)
libagm_mixer_passthrough_plugin_la_LDFLAGS  = -ltinyalsa -lsndcardparser -avoid-version -shared
libagm_mixer_passthrough_plugin_la_LDFLAGS += -lagmclient
libagm_mixer_passthrough_plugin_la_LDFLAGS += -laudio_log_utils
