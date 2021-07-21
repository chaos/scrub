/*****************************************************************************\
 *  $Id:$
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>.
 *  UCRL-CODE-2003-006.
 *
 *  This file is part of Scrub, a program for erasing disks.
 *  For details, see https://code.google.com/p/diskscrub/
 *
 *  Scrub is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  Scrub is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Scrub; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

/* pat - generate pattern on stdout */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#include "getsize.h"

void usage(void);

#define OPTIONS "Sp:s:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"pattern",          required_argument,  0, 'p'},
    {"device-size",      required_argument,  0, 's'},
    {"no-signature",     no_argument,        0, 'S'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

#define SCRUB_MAGIC "\001\002\003SCRUBBED!"

char *prog;

int main(int argc, char *argv[])
{
    extern int optind;
    extern char *optarg;
    int c;
    off_t size = 0;
    int Sopt = 0;
    unsigned char pat = 0;

    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'S':   /* --no-signature */
                Sopt = 1;
                break;
            case 'p':   /* --pattern */
                pat = strtoul(optarg, NULL, 0);
                break;
            case 's':   /* --device-size */
                size = str2size(optarg);
                if (size == 0) {
                    fprintf(stderr, "%s: error parsing size string\n", prog);
                    exit(1);
                }
                break;
            default:
                usage();
        }
    }
    if (size == 0)
        usage();

    if (size < strlen(SCRUB_MAGIC) + 1) {
        fprintf(stderr, "%s: size is too small\n", prog);
        exit(1);
    }
    if (!Sopt) {
        printf("%s", SCRUB_MAGIC);
        size -= strlen(SCRUB_MAGIC);
        putchar(0);
        size--;
    }
    while (size-- > 0)
        putchar(pat);
    exit(0);
}

void usage(void)
{
    fprintf(stderr, "Usage: pat [-S] -s size -p byte\n");
    exit(1);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
