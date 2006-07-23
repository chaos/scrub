/*****************************************************************************\
 *  $Id:$
 *****************************************************************************
 *  Copyright (C) 2005-2006 The Regents of the University of California.
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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if !defined(__FreeBSD__) && !defined(sun)
#include <stdint.h>
#endif
#include <string.h>

#include "util.h"
#include "sig.h"

#define SCRUB_MAGIC "\001\002\003SCRUBBED!"

void
writesig(char *path)
{
    uint8_t buf[sizeof(SCRUB_MAGIC)];
    int fd, n;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        perror(path);
        fprintf(stderr, "error opening file to write signature\n");
        exit(1);
    }
    memcpy(buf, SCRUB_MAGIC, sizeof(buf));
    n = write_all(fd, buf, sizeof(buf));
    if (n < 0) {
        perror(path);
        fprintf(stderr, "error writing signature to file\n");
        exit(1);
    }
    /* ignore short write */
    if (close(fd) < 0) {
        perror(path);
        fprintf(stderr, "error closing file after signature write\n");
        exit(1);
    }
}

int
checksig(char *path)
{
    uint8_t buf[sizeof(SCRUB_MAGIC)];
    int fd, n;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        fprintf(stderr, "error opening file to read signature\n");
        exit(1);
    }
    n = read_all(fd, buf, sizeof(buf));
    if (n < 0) {
        perror(path);
        fprintf(stderr, "error reading signature from file\n");
        exit(1);
    }
    if (close(fd) < 0) {
        perror(path);
        fprintf(stderr, "error closing file after signature read\n");
        exit(1);
    }
    /* short read = no sig */
    if (n == sizeof(buf) && memcmp(buf, SCRUB_MAGIC, sizeof(buf)) == 0)
        return 1;
    return 0;
}

#ifdef STAND
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: sig filename\n");
        exit(1);
    }
    if (!checksig(argv[1])) {
        fprintf(stderr, "no signature, writing one\n");
        writesig(argv[1]);
    } else {
        fprintf(stderr, "signature present\n");
    }
    exit(0);
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
