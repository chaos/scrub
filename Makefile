#
# Copyright (C) 2000 Regents of the University of California
# See ./DISCLAIMER
#
PROJECT=	scrub
TESTPROGS=	pad genrand progress getsize aestest sig
OBJS= 		scrub.o aes.o genrand.o getsize.o fillfile.o filldentry.o \
		progress.o util.o sig.o
CC=		gcc
CFLAGS=		-O -Wall -g


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
	$(CC) -o $@ genrand.c aes.c util.c -DSTAND
progress: progress.c
	$(CC) -o $@ progress.c -DSTAND
getsize: getsize.c
	$(CC) -o $@ getsize.c -DSTAND
sig: sig.c util.o
	$(CC) -o $@ sig.c util.o -DSTAND
aestest: aes.c
	$(CC) -o $@ aes.c -DTEST
pad: pad.c getsize.o
	$(CC) -o $@ pad.c getsize.o

# formatted man page
scrub.1.cat: scrub.1
	nroff -man scrub.1 | col -b >$@
scrub.1.lpr: scrub.1
	nroff -man scrub.1 >$@
