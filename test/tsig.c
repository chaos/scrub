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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>

#include "util.h"
#include "sig.h"

char *prog;

int main(int argc, char *argv[])
{
    bool result;

    prog = basename(argv[0]);
    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", prog);
        exit(1);
    }
    if (checksig (argv[1], &result)) {
        fprintf(stderr, "%s: %s: %s\n", prog, argv[1], strerror(errno));
        exit(1);
    }
    if (!result) {
        fprintf(stderr, "%s: no signature, writing one\n", prog);
        if (writesig(argv[1]) < 0) {
            fprintf(stderr, "%s: %s: %s\n", prog, argv[1], strerror(errno));
            exit(1);
        }
    } else {
        fprintf(stderr, "%s: signature present\n", prog);
    }
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
