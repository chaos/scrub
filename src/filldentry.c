/*****************************************************************************\
 *  Copyright (C) 2001-2007 The Regents of the University of California.
 *  Copyright (C) 2007-2014 Lawrence Livermore National Security, LLC.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov> UCRL-CODE-2003-006
 *
 *  This file is part of Scrub, a program for erasing disks.
 *  For details, see http://code.google.com/p/diskscrub.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the license, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *  See also: http://www.gnu.org/licenses
 *****************************************************************************/

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
