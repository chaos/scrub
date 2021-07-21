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
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

#include "genrand.h"

char *prog;

int main(int argc, char *argv[])
{
    unsigned char buf[24];
    int i, j;

    prog = basename(argv[0]);

    if (initrand() < 0) {
        perror("initrand");
        exit(1);
    }
    for (j = 0; j < 20; j++) {
        genrand(buf, 24);
        for (i = 0; i < 24; i++)
            printf("%-.2hhx", buf[i]);
        printf("\n");
    }
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
