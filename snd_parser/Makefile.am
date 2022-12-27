pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sndparser.pc
EXTRA_DIST = $(pkgconfig_DATA)

AUTOMAKE_OPTIONS = subdir-objects

h_sources  = inc/snd-card-def.h

lib_include_HEADERS = $(h_sources)
lib_includedir = $(includedir)/sndparser/

AM_CFLAGS := -I $(top_srcdir)/inc
AM_CFLAGS += -Wno-unused-parameter

lib_LTLIBRARIES      = libsndcardparser.la
libsndcardparser_la_SOURCES   = src/snd-card-parser.c
libsndcardparser_la_LIBADD = @GLIB_LIBS@ -lexpat -lpthread -lcutils
libsndcardparser_la_CFLAGS := $(AM_CFLAGS)
libsndcardparser_la_CFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
libsndcardparser_la_LDFLAGS   = -avoid-version -shared

if VENDOR_PARTITION
    libsndcardparser_la_CFLAGS += -DCARD_DEF_FILE=\"/vendor/etc/card-defs.xml\"
else
    libsndcardparser_la_CFLAGS += -DCARD_DEF_FILE=\"/etc/card-defs.xml\"
endif
