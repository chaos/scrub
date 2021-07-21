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
    if (read_all(fd, buf, blocksize) < 0)
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
