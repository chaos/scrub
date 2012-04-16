/*****************************************************************************\
 *  $Id:$
 *****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
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

#define SCRUB_MAGIC "\001\002\003SCRUBBED!"

extern char *prog;

/* AIX requires that we write even multiples of blocksize for raw 
 * devices, else EINVAL.
 */
static off_t
sigbufsize(char *path)
{
    struct stat sb;

    if (stat(path, &sb) < 0) {
        fprintf(stderr, "%s: stat %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    return blkalign(strlen(SCRUB_MAGIC), sb.st_blksize, UP);
}

void
writesig(char *path)
{
    unsigned char *buf; 
    int fd, n;
    off_t blocksize = sigbufsize(path);

    buf = malloc(blocksize);
    if (!buf) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    fd = open(path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    n = read_all(fd, buf, blocksize);
    if (n < 0) {
        fprintf(stderr, "%s: read %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (lseek(fd, 0, SEEK_SET) < 0) {
        fprintf(stderr, "%s: lseek %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    memcpy(buf, SCRUB_MAGIC, sizeof(SCRUB_MAGIC));
    n = write_all(fd, buf, blocksize);
    if (n < 0) {
        fprintf(stderr, "%s: write %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (n == 0) {
        fprintf(stderr, "%s: write %s: %s\n", prog, path,
                "returned zero - attempt to write past end of device?");
        exit(1);
    }
    if (close(fd) < 0) {
        fprintf(stderr, "%s: close %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    free(buf);
}

int
checksig(char *path)
{
    unsigned char *buf; 
    int fd, n;
    int result = 0;
    off_t blocksize = sigbufsize(path);

    buf = malloc(blocksize);
    if (!buf) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    n = read_all(fd, buf, blocksize);
    if (n < 0) {
        fprintf(stderr, "%s: read %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (close(fd) < 0) {
        fprintf(stderr, "%s: close %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (n >= strlen(SCRUB_MAGIC)) {
        if (memcmp(buf, SCRUB_MAGIC, strlen(SCRUB_MAGIC)) == 0)
            result = 1;
    }
    memset(buf, 0, blocksize); 
    free(buf);
    return result;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
