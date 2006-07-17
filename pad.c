/*****************************************************************************\
 *  $Id: pad.c 56 2005-11-24 16:32:06Z garlick $
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
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

/* pad - create a sparse file of the indicated size */

#ifdef linux
#define _LARGEFILE_SOURCE 
#define _FILE_OFFSET_BITS 64
#endif

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include "getsize.h"

int main(int argc, char *argv[])
{
    off_t fileOffset;
    char *filename;
    int fd;
    char c = 'x';

    if (argc != 3) {
        fprintf(stderr, "Usage: pad size filename\n");
        exit(1);
    }
    fileOffset = str2size(argv[1]);
    if (fileOffset == 0)
        exit(1);
    filename = argv[2];

    fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror(filename);
        exit(1);
    }
    if (lseek(fd, fileOffset - 1, SEEK_SET) < 0) {
        perror("lseek");
        exit(1);
    }
    if (write(fd, &c, 1) < 0) {
        perror("write");
        exit(1);
    }
    if (close(fd) < 0) {
        perror("close");
        exit(1);
    }

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
