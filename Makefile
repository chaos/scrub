#
# Copyright (C) 2000 Regents of the University of California
# See ./DISCLAIMER
#
# To build scrub with AIX compilers, run:
#   make CC=/usr/vac/bin/cc CFLAGS=-O LDADD=-L/usr/vac/lib
#
PROJECT=	scrub
VERSION         := $(shell awk '/[Vv]ersion:/ {print $$2}' META)
TESTPROGS=	pad genrand progress getsize aestest sig
OBJS= 		scrub.o aes.o genrand.o getsize.o fillfile.o filldentry.o \
		progress.o util.o sig.o
CC=		gcc
CFLAGS=		-O -Wall -g -DSCRUB_VERSION=\"$(VERSION)\"

all: $(PROJECT)
test: $(TESTPROGS)

scrub: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDADD)

clean:
	rm -f $(PROJECT) $(TESTPROGS)
	rm -f $(OBJS)
	rm -f scrub.1.lpr scrub.1.cat

scrub.c genrand.c: genrand.h
genrand.c aes.c: aes.h
scrub.c progress.c: progress.h
scrub.c getsize.c: getsize.h
scrub.c fillfile.c: fillfile.h

# test programs 
genrand: genrand.c aes.c util.c
	$(CC) $(CFLAGS) -o $@ genrand.c aes.c util.c -DSTAND $(LDADD)
progress: progress.c
	$(CC) $(CFLAGS) -o $@ progress.c -DSTAND $(LDADD)
getsize: getsize.c
	$(CC) $(CFLAGS) -o $@ getsize.c -DSTAND $(LDADD)
sig: sig.c util.o
	$(CC) $(CFLAGS) -o $@ sig.c util.o -DSTAND $(LDADD)
aestest: aes.c
	$(CC) $(CFLAGS) -o $@ aes.c -DTEST $(LDADD)
pad: pad.c getsize.o
	$(CC) $(CFLAGS) -o $@ pad.c getsize.o $(LDADD)

# formatted man page
scrub.1.cat: scrub.1
	nroff -man scrub.1 | col -b >$@
scrub.1.lpr: scrub.1
	nroff -man scrub.1 >$@
