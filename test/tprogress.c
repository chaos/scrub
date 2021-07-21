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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <libgen.h>

#include "progress.h"

char *prog;

int
main(int argc, char *argv[])
{
    prog_t p;
    int i;

    prog = basename(argv[0]);

    printf("foo  ");
    progress_create(&p, 70);
    for (i = 1; i <= 100000000L; i++)
        progress_update(p, (double)i/100000000L);
    progress_destroy(p);
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
