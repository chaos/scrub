bin_PROGRAMS = scrub

scrub_SOURCES = \
	filldentry.c \
	filldentry.h \
	fillfile.c \
	fillfile.h \
	genrand.c \
	genrand.h \
	getsize.c \
	getsize.h \
	hwrand.c \
	hwrand.h \
	pattern.c \
	pattern.h \
	progress.c \
	progress.h \
	scrub.c \
	sig.c \
	sig.h \
	util.c \
	util.h

scrub_LDADD = $(LIBPTHREAD) $(LIBPROP)

if LIBGCRYPT
scrub_LDADD += $(gcrypt_LIBS)
else
scrub_SOURCES += aes.c aes.h
endif
