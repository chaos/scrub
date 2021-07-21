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

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else
typedef enum { false, true } bool;
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

typedef enum {
    FILE_NOEXIST,
    FILE_REGULAR,
    FILE_CHAR,
    FILE_BLOCK,
    FILE_OTHER,
} filetype_t;

typedef enum { UP, DOWN } round_t;

int         read_all(int fd, unsigned char *buf, int count);
int         write_all(int fd, const unsigned char *buf, int count);
int         is_symlink(char *path);
filetype_t  filetype(char *path);
off_t       blkalign(off_t offset, int blocksize, round_t rtype);
void *      alloc_buffer(int bufsize);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
