/*****************************************************************************\
 *  $Id: fillfile.h 69 2006-02-14 22:05:36Z garlick $
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

typedef void (*progress_t) (void *arg, double completed);
typedef void (*refill_t) (unsigned char *mem, int memsize);

off_t fillfile(char *path, off_t filesize, unsigned char *mem, int memsize,
        progress_t progress, void *arg, refill_t refill, 
        bool sparse, bool creat);
off_t checkfile(char *path, off_t filesize, unsigned char *mem, int memsize,
        progress_t progress, void *arg, bool sparse);
void  disable_threads(void);
