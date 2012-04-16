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

#define MAXPATBYTES 3
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
void              seq_list(void);
char             *pat2str(pattern_t p);
void              memset_pat(void *s, pattern_t p, size_t n);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
