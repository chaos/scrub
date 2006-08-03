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
writesig(char *path, size_t blocksize)
{
    uint8_t *buf = malloc(blocksize);
    int fd, n;

    if (!buf) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    fd = open(path, O_RDWR);
    if (fd < 0) {
        perror(path);
        fprintf(stderr, "error opening file to write signature\n");
        exit(1);
    }
    memcpy(buf, SCRUB_MAGIC, sizeof(SCRUB_MAGIC));
    /* AIX requires that we write even multiples of blocksize for raw 
     * devices, else EINVAL.
     */
    n = write_all(fd, buf, blocksize);
    if (n < 0) {
        perror(path);
        fprintf(stderr, "error writing signature to file\n");
        exit(1);
    }
    /* Ignore short write.  
     * We'll fail to read the signature next time - not the end of the world.
     */
    if (close(fd) < 0) {
        perror(path);
        fprintf(stderr, "error closing file after signature write\n");
        exit(1);
    }
    free(buf);
}

int
checksig(char *path, size_t blocksize)
{
    uint8_t *buf = malloc(blocksize);
    int fd, n;
    int result = 0;

    if (!buf) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        fprintf(stderr, "error opening file to read signature\n");
        exit(1);
    }
    /* AIX requires that we read even multiples of blocksize for raw
     * devices, else EINVAL.
     */
    n = read_all(fd, buf, blocksize);
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
    /* Treat a short read like "no signature".  Not the end of the world.
     */
    if (n >= strlen(SCRUB_MAGIC)) {
        if (memcmp(buf, SCRUB_MAGIC, strlen(SCRUB_MAGIC)) == 0)
            result = 1;
    }
    free(buf);
    return result;
}

#ifdef STAND
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: sig filename\n");
        exit(1);
    }
    if (!checksig(argv[1]),8192) {
        fprintf(stderr, "no signature, writing one\n");
        writesig(argv[1],8192);
    } else {
        fprintf(stderr, "signature present\n");
    }
    exit(0);
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
