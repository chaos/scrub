/************************************************************\
 * Copyright 2001 The Regents of the University of California.
 * Copyright 2007 Lawrence Livermore National Security, LLC.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of Scrub.
 * For details, see https://github.com/chaos/scrub.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include "getsize.h"

char *prog;

int
main(int argc, char *argv[])
{
    off_t sz;
    struct stat sb;
    char buf[80];

    prog = basename(argv[0]);
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [file|string]\n", prog);
        exit(1);
    }
    if (stat(argv[1], &sb) < 0) {
        if (*argv[1] == '/') {
            fprintf(stderr, "%s: could not stat special file\n", prog);
            exit(1);
        }
        sz = str2size(argv[1]);
	    if (sz == 0) {
                fprintf(stderr, "%s: error parsing size string\n", prog);
                exit(1);
	    }
    } else {
        if (S_ISCHR(sb.st_mode) || S_ISBLK(sb.st_mode)) {
            if (getsize(argv[1], &sz) < 0) {
                fprintf(stderr, "%s: %s: %s\n", prog, argv[1], strerror(errno));
                exit(1);
            }
        } else {
            sz = sb.st_size;
        }
    }
    if (sz != 0) {
        size2str(buf, sizeof(buf), sz);
        printf("%s\n", buf);
    }
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
