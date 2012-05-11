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
static int
sigbufsize(char *path, off_t *blocksize)
{
    struct stat sb;

    if (stat(path, &sb) < 0)
        goto error;
    *blocksize = blkalign(strlen(SCRUB_MAGIC), sb.st_blksize, UP);
    return 0;
error:
    return -1;
}

/* This is a read-modify-write because of AIX requirement above.
 */
int
writesig(char *path)
{
    unsigned char *buf = NULL; 
    int fd = -1, n;
    off_t blocksize;

    if (sigbufsize(path, &blocksize) < 0)
        goto error; 
    if (!(buf = malloc(blocksize)))
        goto nomem;
    if ((fd = open(path, O_RDWR)) < 0)
        goto error;
    if ((n = read_all(fd, buf, blocksize)) < 0)
        goto error;
    memcpy(buf, SCRUB_MAGIC, sizeof(SCRUB_MAGIC));
    if (lseek(fd, 0, SEEK_SET) < 0)
        goto error;
    if ((n = write_all(fd, buf, blocksize)) < 0)
        goto error;
    if (n == 0) {
        errno = EINVAL; /* write past end of device? */
        goto error;
    }
    if (close(fd) < 0)
        goto error;
    free(buf);
    return 0;
nomem:
    errno = ENOMEM;
error:
    if (buf)
        free(buf);
    if (fd != -1)
        (void)close(fd);
    return -1;
}

int
checksig(char *path, bool *status)
{
    unsigned char *buf = NULL;  
    int fd = -1, n;
    bool result = false;
    off_t blocksize;

    if (sigbufsize(path, &blocksize) < 0)
        goto error; 
    if (!(buf = malloc(blocksize)))
        goto nomem;
    if ((fd = open(path, O_RDONLY)) < 0)
        goto error;
    if ((n = read_all(fd, buf, blocksize)) < 0)
        goto error;
    if (n >= strlen(SCRUB_MAGIC)) {
        if (memcmp(buf, SCRUB_MAGIC, strlen(SCRUB_MAGIC)) == 0)
            result = true;
    }
    if (close(fd) < 0)
        goto error;
    free(buf);
    *status = result;
    return 0;
nomem:
    errno = ENOMEM;
error:
    if (buf)
        free (buf);
    if (fd != -1)
        (void)close(fd);
    return -1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
