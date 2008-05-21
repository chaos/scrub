/*****************************************************************************\
 *  $Id: getsize.c 76 2006-02-15 00:49:19Z garlick $
 *****************************************************************************
 *  Copyright (C) 2005 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>.
 *  UCRL-CODE-2003-006.
 *  
 *  This file is part of Scrub, a program for erasing disks.
 *  For details, see <http://www.llnl.gov/linux/scrub/>.
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
            sz = getsize(argv[1]);
            if (sz == 0) {
                    fprintf(stderr, "%s: could not determine device size\n", prog);
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
