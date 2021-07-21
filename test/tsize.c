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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int
main(int argc, char *argv[])
{
    struct stat sb;

    if (argc != 2) {
        fprintf(stderr, "Usage: tsize filename\n");
        exit(1);
    }
    if (stat(argv[1], &sb) < 0) {
        perror(argv[1]);
        exit(1);
    }
    printf("%lld\n", (unsigned long long)sb.st_size);
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
