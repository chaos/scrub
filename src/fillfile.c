/*****************************************************************************\
 *  $Id: fillfile.c 77 2006-02-15 01:00:42Z garlick $
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include <assert.h>

#include "util.h"
#include "fillfile.h"

static int no_threads = 0;

struct memstruct {
    refill_t refill;
    unsigned char *buf;
    int size;
#if WITH_PTHREADS
    pthread_t thd;
    int err;
#endif
};

extern char *prog;

#if defined(O_DIRECT) && (defined(HAVE_POSIX_MEMALIGN) || defined(HAVE_MEMALIGN))
# define MY_O_DIRECT O_DIRECT
#else
# define MY_O_DIRECT 0
#endif

static void *
refill_thread(void *arg)
{
    struct memstruct *mp = (struct memstruct *)arg;

    mp->refill(mp->buf, mp->size);
    return mp;
}

static int
refill_memcpy(struct memstruct *mp, unsigned char *mem, int memsize,
              int filesize, int written)
{
#if WITH_PTHREADS
    if (no_threads) {
        mp->size = memsize;
        refill_thread (mp);
    } else {
        if ((mp->err = pthread_join(mp->thd, NULL))) {
            errno = mp->err;
            goto error;
        }
        assert (memsize == mp->size);
    }
#else
    mp->size = memsize;
    refill_thread (mp);
#endif
    memcpy(mem, mp->buf, memsize);
#if WITH_PTHREADS
    if (!no_threads) {
        written += memsize;
        if (filesize - written > 0) {
            if (mp->size > filesize - written)
                mp->size = filesize - written;
            if ((mp->err = pthread_create(&mp->thd, NULL, refill_thread, mp))) {
                errno = mp->err;
                goto error;
            }
        }
    }
#endif
    return 0;
error:
    return -1;
}

static int
refill_init(struct memstruct **mpp, refill_t refill, int memsize)
{
    struct memstruct *mp = NULL;

    if (!(mp = malloc(sizeof(struct memstruct))))
        goto nomem;
    if (!(mp->buf = malloc(memsize)))
        goto nomem;
    mp->size = memsize;
    mp->refill = refill;
#if WITH_PTHREADS
    if (!no_threads) {
        if ((mp->err = pthread_create(&mp->thd, NULL, refill_thread, mp))) {
            errno = mp->err;
            goto error;
        }
    }
#endif
    *mpp = mp;
    return 0;
nomem:
    errno = ENOMEM;
error:
    return -1;
}

static void
refill_fini(struct memstruct *mp)
{
#if WITH_PTHREADS
    if (!no_threads)
        (void)pthread_join(mp->thd, NULL);
#endif
    free (mp->buf);
    free (mp);
}

/* Fill file (can be regular or special file) with pattern in mem.
 * Writes will use memsize blocks.
 * If 'refill' is non-null, call it before each write (for random fill).
 * If 'progress' is non-null, call it after each write (for progress meter).
 * If 'sparse' is true, only scrub first and last blocks (for testing).
 * The number of bytes written is returned.
 * If 'creat' is true, open with O_CREAT and allow ENOSPC to be non-fatal.
 */
off_t
fillfile(char *path, off_t filesize, unsigned char *mem, int memsize,
         progress_t progress, void *arg, refill_t refill, 
         bool sparse, bool creat)
{
    int fd = -1;
    off_t n;
    off_t written = 0LL;
    int openflags = O_WRONLY;
    struct memstruct *mp = NULL;

    if (filetype(path) != FILE_CHAR)
        openflags |= MY_O_DIRECT;
    if (creat)
        openflags |= O_CREAT;
    fd = open(path, openflags, 0644);
    if (fd < 0 && errno == EINVAL && openflags & MY_O_DIRECT) {
        /* Try again without (MY_)O_DIRECT */
        openflags &= ~MY_O_DIRECT;
        fd = open(path, openflags, 0644);
    }
    if (fd < 0)
        goto error;
    do {
        if (written + memsize > filesize)
            memsize = filesize - written;
        if (refill && !sparse) {
            if (!mp)
                if (refill_init(&mp, refill, memsize) < 0)
                    goto error;
            if (refill_memcpy(mp, mem, memsize, filesize, written) < 0)
                goto error;
        }
        if (sparse && !(written == 0) && !(written + memsize == filesize)) {
            if (lseek(fd, memsize, SEEK_CUR) < 0)
                goto error;
            written += memsize;
        } else {
            n = write_all(fd, mem, memsize);
            if (creat && n < 0 && errno == ENOSPC)
                break;
            if (n == 0) {
                errno = EINVAL; /* write past end of device? */
                goto error;
            } else if (n < 0)
                goto error;
            written += n;
        }
        if (progress)
            progress(arg, (double)written/filesize);
    } while (written < filesize);
    if (fsync(fd) < 0)
        goto error;
#if defined(HAVE_POSIX_FADVISE) && defined(POSIX_FADV_DONTNEED)
    /* Try to fool the kernel into dropping any device cache */
    (void)posix_fadvise(fd, 0, filesize, POSIX_FADV_DONTNEED);
#endif
    if (close(fd) < 0)
        goto error;
    if (mp)
        refill_fini(mp);
    return written;
error:
    if (mp)
        refill_fini(mp);
    if (fd != -1)
        (void)close(fd);
    return (off_t)-1;
}

/* Verify that file was filled with 'mem' patterns.
 */
off_t
checkfile(char *path, off_t filesize, unsigned char *mem, int memsize,
          progress_t progress, void *arg, bool sparse)
{
    int fd = -1;
    off_t n;
    off_t verified = 0LL;
    unsigned char *buf = NULL;
    int openflags = O_RDONLY;

    if (!(buf = alloc_buffer(memsize)))
        goto nomem;
    if (filetype(path) != FILE_CHAR)
        openflags |= MY_O_DIRECT;
    fd = open(path, openflags);
    if (fd < 0 && errno == EINVAL && openflags & MY_O_DIRECT) {
        /* Try again without (MY_)O_DIRECT */
        openflags &= ~MY_O_DIRECT;
        fd = open(path, openflags);
    }
    if (fd < 0)
        goto error;
    do {
        if (verified + memsize > filesize)
            memsize = filesize - verified;
        if (sparse && !(verified == 0) && !(verified + memsize == filesize)) {
            if (lseek(fd, memsize, SEEK_CUR) < 0)
                goto error;
            verified += memsize;
        } else {
            n = read_all(fd, buf, memsize);
            if (n < 0)
                goto error;
            if (n == 0) {
                errno = EINVAL; /* early EOF */
                goto error;
            }
            if (memcmp(mem, buf, memsize) != 0) {
                break; /* return < filesize means verification failure */
            }
            verified += n;
        }
        if (progress)
            progress(arg, (double)verified/filesize);
    } while (verified < filesize);
    if (close(fd) < 0)
        goto error;
    free(buf);
    return verified;
nomem:
    errno = ENOMEM;
error:
    if (buf)
        free (buf);
    if (fd != -1)
        (void)close(fd);
    return (off_t)-1;
}

void
disable_threads(void)
{
    no_threads = 1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
