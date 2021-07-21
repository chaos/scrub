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
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>

#include "util.h"

/* Handles short reads but otherwise just like read(2).
 */
int
read_all(int fd, unsigned char *buf, int count)
{
    int n;

    do {
        n = read(fd, buf, count);
        if (n > 0) {
            count -= n;
            buf += n;
        }
    } while (n > 0 && count > 0);

    return n;
}

/* Handles short writes but otherwise just like write(2).
 */
int
write_all(int fd, const unsigned char *buf, int count)
{
    int n;

    do {
        n = write(fd, buf, count);
        if (n > 0) {
            count -= n;
            buf += n;
        }
    } while (n > 0 && count > 0);

    return n;
}

/* Indicates whether the file represented by 'path' is a symlink.
 */
int
is_symlink(char *path)
{
    struct stat sb;
    return lstat(path, &sb) == 0 && S_ISLNK(sb.st_mode);
}

/* Return the type of file represented by 'path'.
 */
filetype_t
filetype(char *path)
{
    struct stat sb;

    filetype_t res = FILE_NOEXIST;

    if (stat(path, &sb) == 0) {
        if (S_ISREG(sb.st_mode))
            res = FILE_REGULAR;
        else if (S_ISCHR(sb.st_mode))
            res = FILE_CHAR;
        else if (S_ISBLK(sb.st_mode))
            res = FILE_BLOCK;
        else
            res = FILE_OTHER;
    }
    return res;
}

/* Round 'offset' up to an even multiple of 'blocksize'.
 */
off_t
blkalign(off_t offset, int blocksize, round_t rtype)
{
    off_t r = offset % blocksize;

    if (r > 0) {
        switch (rtype) {
            case UP:
                offset += (blocksize - r);
                break;
            case DOWN:
                offset -= r;
                break;
        }
    }

    return offset;
}

/* Allocate an aligned buffer
 */
#define ALIGNMENT	(16*1024*1024) /* Hopefully good enough */
void *
alloc_buffer(int bufsize)
{
    void *ptr;

#ifdef HAVE_POSIX_MEMALIGN
    int err = posix_memalign(&ptr, ALIGNMENT, bufsize);
    if (err) {
        errno = err;
        ptr = NULL;
    }
#elif defined(HAVE_MEMALIGN)
    ptr = memalign(ALIGNMENT, bufsize);
#else
    ptr = malloc(bufsize);	/* Hope for the best? */
#endif

    return ptr;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
