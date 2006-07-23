/*****************************************************************************\
 *  $Id: filldentry.c 82 2006-02-15 10:20:25Z garlick $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#if !defined(__FreeBSD__) && !defined(sun)
#include <stdint.h>
#endif
#include <assert.h>

#include "filldentry.h"
#include "util.h"


/* fsync(2) directory.
 */
static void 
dirsync(char *dir)
{
#if defined(_AIX)
    sync();
#else
    int fd;

    fd = open(dir, O_RDONLY);
    if (fd < 0) {
        perror(dir);
        exit(1);
    }
    if (fsync(fd) < 0) {
        perror("fsync");
        exit(1);
    }
    if (close(fd) < 0) {
        perror("close");
        exit(1);
    }
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
void 
filldentry(char *path, int pat)
{
    char *new = newname(path, pat);
    char *cpy = strdup(path);
    char *dir;

    assert(strlen(cpy) == strlen(new));
   
    if (!cpy || !new) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    if (rename(path, new) == -1) {
        perror("rename");
        exit(1);
    }

    dir = dirname(cpy);
    dirsync(dir);
    free(cpy);

    assert(strlen(path) == strlen(new));
    strcpy(path, new);
    free(new);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
