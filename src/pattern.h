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

#include <stdio.h>

#define MAXPATBYTES 16
#define MAXSEQPATTERNS 35
typedef enum {
    PAT_NORMAL,
    PAT_RANDOM,
    PAT_VERIFY,
} ptype_t;
typedef struct {
    ptype_t     ptype;
    int         len;
    int         pat[MAXPATBYTES];
} pattern_t;

typedef struct {
    char       *key;
    char       *desc;
    int         len;
    pattern_t   pat[MAXSEQPATTERNS];
} sequence_t;

const sequence_t *seq_lookup(char *name);
void              seq_list(FILE *fp);
char             *pat2str(pattern_t p);
void              memset_pat(void *s, pattern_t p, size_t n);

const sequence_t *seq_lookup_byindex(int i);
const int         seq_count(void);
void              seq2str(const sequence_t *sp, char *buf, int len);

sequence_t *seq_create (char *key, char *desc, char *s);
void seq_destroy (sequence_t *sp);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
