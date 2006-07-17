/*****************************************************************************\
 *  $Id: fillfile.c 77 2006-02-15 01:00:42Z garlick $
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

#if defined(linux) || defined(sun)
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64
#endif
 
#if defined(_AIX)
#define _LARGE_FILES
#include <sys/mode.h>
#endif
 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__FreeBSD__) && !defined(sun)
#include <stdint.h>
#endif

#include "fillfile.h"
#include "util.h"

static void 
_fsync_if_file(int fd)
{
    struct stat sb;

    if (fstat(fd, &sb) < 0) {
        perror("fstat");
        exit(1);
    }
    if (S_ISREG(sb.st_mode)) {
        if (fsync(fd) < 0) {
            perror("fsync");
            exit(1);
        }
    }
}

/* Fill file (can be regular or special file) with pattern in mem.
 * Writes will use memsize blocks.
 * If 'refill' is non-null, call it before each write (for random fill).
 * If 'progress' is non-null, call it after each write (for progress meter).
 * The number of bytes written is returned.
 */
off_t
fillfile(char *path, off_t filesize, uint8_t *mem, size_t memsize,
        progress_t progress, void *arg, refill_t refill)
{
    int fd;
    off_t n;
    off_t res = 0LL;

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror(path);
        exit(1);
    }

    do {
        if (refill)
            refill(mem, memsize);
        if (res + memsize > filesize)
            memsize = filesize - res;
        n = write_all(fd, mem, memsize);
        if (n < 0) {
            perror("write");
            exit(1);
        }
        res += n;
        if (progress)
            progress(arg, (double)res/filesize);
    } while (res < filesize);

    _fsync_if_file(fd);

    if (close(fd) < 0) {
        perror("close");
        exit(1);
    }

    return res;
}

/* Verify that file was filled with 'mem' patterns.
 */
off_t
checkfile(char *path, off_t filesize, uint8_t *mem, size_t memsize,
        progress_t progress, void *arg)
{
    int fd;
    off_t n;
    off_t res = 0LL;
    uint8_t *buf;

    buf = (uint8_t *)malloc(memsize);
    if (!buf) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        exit(1);
    }

    do {
        if (res + memsize > filesize)
            memsize = filesize - res;
        n = read_all(fd, buf, memsize);
        if (n < 0) {
            perror("read");
            exit(1);
        }
        if (n == 0) {
            fprintf(stderr, "premature EOF\n");
            exit(1);
        }
        if (memcmp(mem, buf, memsize) != 0) {
            fprintf(stderr, "verification failure\n");
            exit(1);
        }
        res += n;
        if (progress)
            progress(arg, (double)res/filesize);
    } while (res < filesize);

    if (close(fd) < 0) {
        perror("close");
        exit(1);
    }

    free(buf);

    return res;
}

/* Create file and fill it with pattern in mem until the file system is full.
 * Writes will use memsize blocks.
 * The number of bytes written is returned.
 */
off_t
growfile(char *path, uint8_t *mem, size_t memsize, refill_t refill)
{
    int fd;
    off_t n;
    off_t res = 0LL;

    fd = open(path, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        perror(path);
        exit(1);
    }

    do {
        if (refill)
            refill(mem, memsize);
        n = write_all(fd, mem, memsize);
        if (n < 0 && errno == ENOSPC && memsize > 1) {
            memsize = 1;
            continue;
        }
        if (n < 0 && errno != ENOSPC) {
            perror(path);
            exit(1);
        }
        if (n == 0) {
            fprintf(stderr, "write returned 0, aborting\n");
            exit(1);
        }
        if (n > 0)
            res += n;
    } while (n > 0);

    _fsync_if_file(fd);

    if (close(fd) < 0) {
        perror("close");
        exit(1);
    }

    return res;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
