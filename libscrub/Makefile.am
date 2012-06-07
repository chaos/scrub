AM_CPPFLAGS = -I$(top_srcdir)/src

lib_LTLIBRARIES = libscrub.la

noinst_PROGRAMS = tscrub

tscrub_SOURCES = \
	tscrub.c

tscrub_LDADD = libscrub.la $(LIBPTHREAD)

libscrub_la_SOURCES = \
	libscrub.c \
	scrub.h \
	../src/aes.c \
	../src/filldentry.c \
	../src/fillfile.c \
	../src/genrand.c \
	../src/getsize.c \
	../src/hwrand.c \
	../src/pattern.c \
	../src/progress.c \
	../src/sig.c \
	../src/util.c

libscrub_la_LDFLAGS = -module -avoid-version -Wl,--version-script=libscrub.map

if WITH_PKG_CONFIG
pkgconfig_DATA = libscrub.pc
endif
include_HEADERS = scrub.h

EXTRA_DIST = libscrub.map
