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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>

#include "filldentry.h"
#include "util.h"

extern char *prog;


/* fsync(2) directory.
 */
static int
dirsync(char *dir)
{
#if defined(_AIX) /* FIXME: need HAVE_FSYNC_DIR macro */
    sync();
    return 0;
#else
    int fd = -1;

    if ((fd = open(dir, O_RDONLY)) < 0)
        goto error;
    if (fsync(fd) < 0)
        goto error;
    if (close(fd) < 0)
        goto error;
    return 0;
error:
    if (fd != -1)
        (void)close(fd);
    return -1;
#endif
}

/* Construct a new name for 'old' by replacing its file component with
 * a string of 'pat' characters.  Caller must free result.
 */
static char *
newname(char *old, int pat)
{
    char *new;
    char *base;

    assert(old != NULL);
    assert(strlen(old) > 0);
    assert(pat != 0);
    assert(pat <= 0xff);

    new = strdup(old);
    if (new) {
        if ((base = strrchr(new, '/')))
            base++;
        else
            base = new;
        memset(base, pat, strlen(base));
    }

    return new;
}

/* Rename file to pattern and fsync the directory.
 * Modifies 'path' so it is valid on successive calls.
 */
int
filldentry(char *path, int pat)
{
    char *new = NULL;
    char *cpy = NULL;
    char *dir;

    if (!(new = newname(path, pat)))
        goto nomem;
    if (!(cpy = strdup(path)))
        goto nomem;
    assert(strlen(cpy) == strlen(new));
    if (rename(path, new) == -1)
        goto error;
    dir = dirname(cpy);
    if (dirsync(dir) < 0)
        goto error;
    free(cpy);
    assert(strlen(path) == strlen(new));
    strcpy(path, new);
    free(new);
    return 0;
nomem:
    errno = ENOMEM;
error:
    if (cpy)
        free(cpy);
    if (new)
        free(new);
    return -1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
