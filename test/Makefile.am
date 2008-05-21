check_PROGRAMS = pad trand aestest tprogress tgetsize tsig

AM_CFLAGS = -I$(top_srcdir)/src

common_sources = \
	$(top_srcdir)/src/getsize.c \
	$(top_srcdir)/src/genrand.c \
	$(top_srcdir)/src/aes.c \
	$(top_srcdir)/src/util.c \
	$(top_srcdir)/src/progress.c \
	$(top_srcdir)/src/sig.c

pad_SOURCES = pad.c $(common_sources)
trand_SOURCES = trand.c $(common_sources)
aestest_SOURCES = aestest.c $(common_sources)
tprogress_SOURCES = tprogress.c $(common_sources)
tgetsize_SOURCES = tgetsize.c $(common_sources)
tsig_SOURCES = tsig.c $(common_sources)
