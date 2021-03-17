SUBDIRS = src man test

if LIBSCRUB
SUBDIRS += libscrub
endif

dist_doc_DATA = \
	AUTHORS \
	ChangeLog \
	COPYING \
	DISCLAIMER \
	INSTALL \
	NEWS \
	README \
	README.md

scrubdir = $(datadir)/scrub
scrub_DATA = scrub.conf.sample

EXTRA_DIST = META scrub.spec
