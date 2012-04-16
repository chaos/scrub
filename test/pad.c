/*****************************************************************************\
 *  $Id: pad.c 56 2005-11-24 16:32:06Z garlick $
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

/* pad - create a sparse file of the indicated size */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <assert.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>

#include "getsize.h"

char *prog;

/* N.B. we want to notice if large file support isn't working and we try
 * to pad a >4G file.  Some other causes of failure are fsize ulimit and
 * lack of large file support on underlying file system (AIX JFS requires
 * a flag at mkfs time for example).
 */


static void
check_rlimit_fsize(off_t size)
{
    struct rlimit r;

    if (getrlimit(RLIMIT_FSIZE, &r) < 0) {
        fprintf(stderr, "%s: getrlimit: %s\n", prog, strerror(errno));
        exit(1);
    }
    if (r.rlim_cur == RLIM_INFINITY || r.rlim_cur >= size)
        return;
    if (r.rlim_max < size) {
        fprintf(stderr, "%s: increase your RLIMIT_FSIZE\n", prog);
        exit(1);
    }
    r.rlim_cur = r.rlim_max;
    if (setrlimit(RLIMIT_FSIZE, &r) < 0) {
        fprintf(stderr, "%s: setrlimit: %s\n", prog, strerror(errno));
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    off_t fileOffset;
    char *filename;
    int fd;
    char c = 'x';

    assert(sizeof(off_t) == 8);

    prog = basename(argv[0]);
    if (argc != 3) {
        fprintf(stderr, "Usage: %s size filename\n", prog);
        exit(1);
    }
    fileOffset = str2size(argv[1]);
    if (fileOffset == 0) {
        fprintf(stderr, "%s: error parsing size string\n", prog);
        exit(1);
    }
    filename = argv[2];

    check_rlimit_fsize(fileOffset);

    fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, filename, strerror(errno));
        exit(1);
    }
    if (lseek(fd, fileOffset - 1, SEEK_SET) < 0) {
        fprintf(stderr, "%s: lseek %lld %s: %s\n", prog, 
            (long long)fileOffset - 1, filename, strerror(errno));
        exit(1);
    }
    if (write(fd, &c, 1) < 0) {
        fprintf(stderr, "%s: write %s: %s\n", prog, filename, strerror(errno));
        exit(1);
    }
    if (close(fd) < 0) {
        fprintf(stderr, "%s: close %s: %s\n", prog, filename, strerror(errno));
        exit(1);
    }

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
