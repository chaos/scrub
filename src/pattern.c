/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
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
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pattern.h"

/* N.B.
 * The number of patterns in a sequence is limited to MAXSEQPATTERNS.
 * The number of bytes in a pattern is limited to MAXPATBYTES.
 * Just change these constants in pattern.h if you need more.
 */ 

/* See assertions in scrub.c::scrub_dirent() for limits
 * on this pattern sequence.
 */
static const sequence_t dirent_seq = { 
    "dirent", "dirent", 6, {
        { NORMAL, 1, {0x55} }, 
        { NORMAL, 1, {0x22} }, 
        { NORMAL, 1, {0x55} }, 
        { NORMAL, 1, {0x22} }, 
        { NORMAL, 1, {0x55} }, 
        { NORMAL, 1, {0x22} }, 
    },
};

static const sequence_t old_seq = { 
    "old", "pre v1.7 scrub", 5, {
        { NORMAL, 1, {0x00} }, 
        { NORMAL, 1, {0xff} }, 
        { NORMAL, 1, {0xaa} }, 
        { RANDOM, 0, {0x00} },
        { VERIFY, 1, {0x55} }, 
    },
};

static const sequence_t fastold_seq = { 
    "fastold", "pre v1.7 scrub (skip random)", 4, {
        { NORMAL, 1, {0x00} }, 
        { NORMAL, 1, {0xff} }, 
        { NORMAL, 1, {0xaa} }, 
        { VERIFY, 1, {0x55} }, 
    },
};

static const sequence_t nnsa_seq = { 
    "nnsa", "NNSA NAP-14.x", 3, {
        { RANDOM, 0, {0x00} },
        { RANDOM, 0, {0x00} },
        { VERIFY, 1, {0x00} }, 
    },
};

static const sequence_t dod_seq = { 
    "dod", "DoD 5220.22-M", 4, {
        { NORMAL, 1, {0x00} }, 
        { NORMAL, 1, {0xff} }, 
        { RANDOM, 0, {0x00} },
        { VERIFY, 1, {0x00} }, 
    },
};

static const sequence_t bsi_seq = { 
    "bsi", "BSI", 9, {
        { NORMAL, 1, {0xff} }, 
        { NORMAL, 1, {0xfe} }, 
        { NORMAL, 1, {0xfd} }, 
        { NORMAL, 1, {0xfb} }, 
        { NORMAL, 1, {0xf7} },
        { NORMAL, 1, {0xef} }, 
        { NORMAL, 1, {0xdf} }, 
        { NORMAL, 1, {0xbf} }, 
        { NORMAL, 1, {0x7f} },
    },
};

static const sequence_t gutmann_seq = {
    "gutmann", "Gutmann", 35, {
        { RANDOM, 0, {0x00} },
        { RANDOM, 0, {0x00} },
        { RANDOM, 0, {0x00} },
        { RANDOM, 0, {0x00} },
        { NORMAL, 1, {0x55} }, 
        { NORMAL, 1, {0xaa} }, 
        { NORMAL, 3, {0x92, 0x49, 0x24} },
        { NORMAL, 3, {0x49, 0x24, 0x92} },
        { NORMAL, 3, {0x24, 0x92, 0x49} },
        { NORMAL, 1, {0x00} }, 
        { NORMAL, 1, {0x11} }, 
        { NORMAL, 1, {0x22} }, 
        { NORMAL, 1, {0x33} }, 
        { NORMAL, 1, {0x44} }, 
        { NORMAL, 1, {0x55} }, 
        { NORMAL, 1, {0x66} }, 
        { NORMAL, 1, {0x77} }, 
        { NORMAL, 1, {0x88} }, 
        { NORMAL, 1, {0x99} }, 
        { NORMAL, 1, {0xaa} }, 
        { NORMAL, 1, {0xbb} }, 
        { NORMAL, 1, {0xcc} }, 
        { NORMAL, 1, {0xdd} }, 
        { NORMAL, 1, {0xee} }, 
        { NORMAL, 1, {0xff} }, 
        { NORMAL, 3, {0x92, 0x49, 0x24} },
        { NORMAL, 3, {0x49, 0x24, 0x92} },
        { NORMAL, 3, {0x24, 0x92, 0x49} },
        { NORMAL, 3, {0x6d, 0xb6, 0xdb} },
        { NORMAL, 3, {0xb6, 0xdb, 0x6d} },
        { NORMAL, 3, {0xdb, 0x6d, 0xb6} },
        { RANDOM, 0, {0x00} },
        { RANDOM, 0, {0x00} },
        { RANDOM, 0, {0x00} },
        { RANDOM, 0, {0x00} },
    },
};

static const sequence_t random_seq = { 
    "random", "One Random Pass", 1, {
        { RANDOM, 0, {0x00} },
    },
};

static const sequence_t random2_seq = { 
    "random2", "Two Random Passes", 2, {
        { RANDOM, 0, {0x00} },
        { RANDOM, 0, {0x00} },
    },
};

static const sequence_t *sequences[] = {
	&dirent_seq,
	&fastold_seq,
	&old_seq,
	&dod_seq,
	&nnsa_seq,
	&bsi_seq,
	&gutmann_seq,
	&random_seq,
	&random2_seq,
};	

const sequence_t *
seq_lookup(char *key)
{
    int i;

    for (i = 0; i < sizeof(sequences)/sizeof(sequences[0]); i++)
        if (!strcmp(sequences[i]->key, key))
            return sequences[i];
    return NULL;
}

void
memset_pat(void *s, pattern_t p, size_t n)
{
    int i;
    char *sp = (char *)s;

    for (i = 0; i < n; i++)
        sp[i] = p.pat[i % p.len];
}

char *
pat2str(pattern_t p)
{
    static char str[MAXPATBYTES*2 + 3];
    int i;

    assert(sizeof(str) > p.len*2 + 2);
    switch (p.ptype) {
        case RANDOM:
            snprintf(str, sizeof(str), "random");
        case VERIFY:
        case NORMAL:
            snprintf(str, sizeof(str), "0x");
            for (i = 0; i < p.len; i++)
                snprintf(str+2*i+2, sizeof(str)-2*i-2, "%.2x", p.pat[i]);
            break;
    }
    return str;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
