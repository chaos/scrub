/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>.
 *  UCRL-CODE-2003-006.
 *  
 *  This file is part of Scrub, a program for erasing disks.
 *  For details, see https://code.google.com/p/diskscrub/
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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "pattern.h"

extern char *prog;

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
        { PAT_NORMAL, 1, {0x32} },
        { PAT_NORMAL, 1, {0x4d} },
        { PAT_NORMAL, 1, {0x32} },
        { PAT_NORMAL, 1, {0x4d} },
        { PAT_NORMAL, 1, {0x32} },
        { PAT_NORMAL, 1, {0x4d} },
    },
};

static const sequence_t old_seq = { 
    "old", "pre v1.7 scrub", 5, {
        { PAT_NORMAL, 1, {0x00} }, 
        { PAT_NORMAL, 1, {0xff} }, 
        { PAT_NORMAL, 1, {0xaa} }, 
        { PAT_RANDOM, 0, {0x00} },
        { PAT_VERIFY, 1, {0x55} }, 
    },
};

static const sequence_t fastold_seq = { 
    "fastold", "pre v1.7 scrub (skip random)", 4, {
        { PAT_NORMAL, 1, {0x00} }, 
        { PAT_NORMAL, 1, {0xff} }, 
        { PAT_NORMAL, 1, {0xaa} }, 
        { PAT_VERIFY, 1, {0x55} }, 
    },
};

static const sequence_t nnsa_seq = { 
    "nnsa", "NNSA NAP-14.1-C", 3, {
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_VERIFY, 1, {0x00} }, 
    },
};

static const sequence_t dod_seq = { 
    "dod", "DoD 5220.22-M", 3, {
        { PAT_RANDOM, 0, {0x00} },
        { PAT_NORMAL, 1, {0x00} }, 
        { PAT_VERIFY, 1, {0xff} }, 
    },
};

static const sequence_t bsi_seq = { 
    "bsi", "BSI", 9, {
        { PAT_NORMAL, 1, {0xff} }, 
        { PAT_NORMAL, 1, {0xfe} }, 
        { PAT_NORMAL, 1, {0xfd} }, 
        { PAT_NORMAL, 1, {0xfb} }, 
        { PAT_NORMAL, 1, {0xf7} },
        { PAT_NORMAL, 1, {0xef} }, 
        { PAT_NORMAL, 1, {0xdf} }, 
        { PAT_NORMAL, 1, {0xbf} }, 
        { PAT_NORMAL, 1, {0x7f} },
    },
};

static const sequence_t gutmann_seq = {
    "gutmann", "Gutmann", 35, {
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_NORMAL, 1, {0x55} }, 
        { PAT_NORMAL, 1, {0xaa} }, 
        { PAT_NORMAL, 3, {0x92, 0x49, 0x24} },
        { PAT_NORMAL, 3, {0x49, 0x24, 0x92} },
        { PAT_NORMAL, 3, {0x24, 0x92, 0x49} },
        { PAT_NORMAL, 1, {0x00} }, 
        { PAT_NORMAL, 1, {0x11} }, 
        { PAT_NORMAL, 1, {0x22} }, 
        { PAT_NORMAL, 1, {0x33} }, 
        { PAT_NORMAL, 1, {0x44} }, 
        { PAT_NORMAL, 1, {0x55} }, 
        { PAT_NORMAL, 1, {0x66} }, 
        { PAT_NORMAL, 1, {0x77} }, 
        { PAT_NORMAL, 1, {0x88} }, 
        { PAT_NORMAL, 1, {0x99} }, 
        { PAT_NORMAL, 1, {0xaa} }, 
        { PAT_NORMAL, 1, {0xbb} }, 
        { PAT_NORMAL, 1, {0xcc} }, 
        { PAT_NORMAL, 1, {0xdd} }, 
        { PAT_NORMAL, 1, {0xee} }, 
        { PAT_NORMAL, 1, {0xff} }, 
        { PAT_NORMAL, 3, {0x92, 0x49, 0x24} },
        { PAT_NORMAL, 3, {0x49, 0x24, 0x92} },
        { PAT_NORMAL, 3, {0x24, 0x92, 0x49} },
        { PAT_NORMAL, 3, {0x6d, 0xb6, 0xdb} },
        { PAT_NORMAL, 3, {0xb6, 0xdb, 0x6d} },
        { PAT_NORMAL, 3, {0xdb, 0x6d, 0xb6} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
    },
};

static const sequence_t random_seq = { 
    "random", "One Random Pass", 1, {
        { PAT_RANDOM, 0, {0x00} },
    },
};

static const sequence_t random2_seq = { 
    "random2", "Two Random Passes", 2, {
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
    },
};

static const sequence_t schneier_seq = {
    "schneier", "Bruce Schneier Algorithm", 7, {
        { PAT_NORMAL, 1, {0x00} },
        { PAT_NORMAL, 1, {0xff} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
    },
};

static const sequence_t pfitzner7_seq = {
    "pfitzner7", "Roy Pfitzner 7-random-pass method", 7, {
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
    },
};

static const sequence_t pfitzner33_seq = {
    "pfitzner33", "Roy Pfitzner 33-random-pass method", 33, {
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
        { PAT_RANDOM, 0, {0x00} },
    },
};

static const sequence_t usarmy_seq = { 
    "usarmy", "US Army AR380-19", 3, {
        { PAT_NORMAL, 1, {0x00} }, 
        { PAT_NORMAL, 1, {0xff} }, 
        { PAT_RANDOM, 0, {0x00} },
    },
};

static const sequence_t fillzero_seq = {
    "fillzero", "Quick Fill with 0x00", 1, {
        { PAT_NORMAL, 1, {0x00} },
    },
};

static const sequence_t fillff_seq = {
    "fillff", "Quick Fill with 0xff", 1, {
        { PAT_NORMAL, 1, {0xff} },
    },
};

static const sequence_t custom_seq = {
    "custom", "custom=\"str\" 16 chr max, C esc like \\r, \\xFF, \\377, \\\\", 1, {
    },
};

static const sequence_t *sequences[] = {
	&nnsa_seq,
	&dod_seq,
	&bsi_seq,
	&usarmy_seq,
	&random_seq,
	&random2_seq,
	&schneier_seq,
    &pfitzner7_seq,
    &pfitzner33_seq,
	&gutmann_seq,
	&fastold_seq,
	&old_seq,
	&dirent_seq,
	&fillzero_seq,
	&fillff_seq,
};

const int
seq_count(void)
{
    return sizeof(sequences)/sizeof(sequences[0]);
}

#define isoctdigit(c)  (((c) >= '0' && (c) <= '7'))

#define ishexdigit(c)  (((c) >= '0' && (c) <= '9') \
                     || ((c) >= 'a' && (c) <= 'f') \
                     || ((c) >= 'A' && (c) <= 'F'))

/* Expand C style numerical escapes: \nnn (octal), \xnn (hex), and \\.
 */
static int 
strtomem (int *data, int len, char *s)
{
    char tmp[16];
    int i = 0;

    while (*s && i < len) {
        if  (*s == '\\' && *(s + 1) == '\\') {
            data[i++] = *s;
            s += 2;
            continue;
        }
        if (*s == '\\' && strlen (s) >= 4 && *(s + 1) == 'x'
                       && ishexdigit(*(s + 2)) && ishexdigit(*(s + 3))) {
            memcpy (tmp, s + 2, 2);
            tmp[2] = '\0';
            data[i++] = strtoul (tmp, NULL, 16);
            s += 4;
            continue;
        }
        if (*s == '\\' && strlen (s) >= 4 && isoctdigit (*(s + 1))
                       && isoctdigit (*(s + 2)) && isoctdigit (*(s + 3))) {
            memcpy (tmp, s + 1, 3);
            tmp[3] = '\0';
            data[i++] = strtoul (tmp, NULL, 8) & 0xff;
            s += 4;
            continue;
        }
        data[i++] = *s++;
    }
    return (*s ? -1 : i);
}

void
seq_destroy (sequence_t *sp)
{
    if (sp->key)
        free (sp->key);
    if (sp->desc)
        free (sp->desc);
    free (sp);
}

sequence_t *
seq_create (char *key, char *desc, char *s)
{
    sequence_t *sp = NULL;
    int len;

    if (!(sp = malloc (sizeof (*sp))))
        goto nomem;
    memset (sp, 0, sizeof (*sp));
    if (!(sp->key = strdup (key)))
        goto nomem;
    if (!(sp->desc = strdup (desc)))
        goto nomem;
    sp->len = 1;
    len = strtomem (sp->pat[0].pat, MAXPATBYTES, s);
    if (len < 0) {
        errno = EINVAL;
        goto error;
    }
    sp->pat[0].len = len;
    return sp;
nomem:
    errno = ENOMEM;
error:
    if (sp)
        seq_destroy (sp);
    return NULL;
}

const sequence_t *
seq_lookup(char *key)
{
    const int len = seq_count();
    const sequence_t *seq = NULL;
    int i;

    for (i = 0; i < len; i++) {
        if (!strcmp(sequences[i]->key, key)) {
            seq = sequences[i];
            break;
        }
    }
    return seq;
}

const sequence_t *
seq_lookup_byindex (int i)
{
    assert (i >= 0 && i < seq_count());
    return sequences[i];
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
    int len = p.len;

    if (len > MAXPATBYTES)
        len = MAXPATBYTES;
    switch (p.ptype) {
        case PAT_RANDOM:
            snprintf(str, sizeof(str), "random");
            break;
        case PAT_VERIFY:
        case PAT_NORMAL:
            snprintf(str, sizeof(str), "0x");
            for (i = 0; i < len; i++)
                snprintf(str+2*i+2, sizeof(str)-2*i-2, "%.2x", p.pat[i]);
            break;
    }
    return str;
}

void
seq2str(const sequence_t *sp, char *buf, int len)
{
    snprintf (buf, len, "  %-10.10s %4.d-pass   %s",
              sp->key, sp->len, sp->desc);
}

void
seq_list(void)
{
    const int len = seq_count();
    char buf[80];
    int i;

    for (i = 0; i < len; i++) {
        seq2str(sequences[i], buf, sizeof(buf));
        fprintf(stderr, "%s\n", buf);
    }
    seq2str(&custom_seq, buf, sizeof(buf));
    fprintf(stderr, "%s\n", buf);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
