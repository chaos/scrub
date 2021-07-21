/*****************************************************************************\
 *  $Id: genrand.c 68 2006-02-14 21:54:16Z garlick $
 *****************************************************************************
 *  Copyright (C) 2001-2005 The Regents of the University of California.
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
