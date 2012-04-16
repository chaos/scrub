/*****************************************************************************\
 *  hwrand.c
 *****************************************************************************
 *  Copyright (C) 2012 Intel Corporation
 *  Written by H. Peter Anvin <h.peter.anvin@intel.com>
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "hwrand.h"

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

#ifdef __i386__

static bool have_eflag(unsigned long flag)
{
    unsigned long t0, t1;

    asm volatile("pushf ; "
                 "pushf ; "
                 "pop %0 ; "
                 "mov %0,%1 ; "
                 "xor %2,%0 ; "
                 "push %0 ; "
                 "popf ; "
                 "pushf ; "
                 "pop %0 ; "
                 "popf"
                 : "=r" (t0), "=r" (t1)
                 : "ri" (flag));

    return !!((t0^t1) & flag);
}

static bool have_cpuid(void)
{
    return have_eflag(1 << 21); /* ID flag */
}

#else  /* x86_64 */

static bool have_cpuid(void)
{
    return true;
}

#endif

struct cpuid {
    unsigned int eax, ecx, edx, ebx;
};

static void cpuid(unsigned int leaf, unsigned int subleaf, struct cpuid *out)
{
#ifdef __i386__
    /* %ebx is a forbidden register if we compile with -fPIC or -fPIE */
    asm volatile("movl %%ebx,%0 ; cpuid ; xchgl %%ebx,%0"
                 : "=r" (out->ebx),
                   "=a" (out->eax),
                   "=c" (out->ecx),
                   "=d" (out->edx)
                 : "a" (leaf), "c" (subleaf));
#else
    asm volatile("cpuid"
                 : "=b" (out->ebx),
                   "=a" (out->eax),
                   "=c" (out->ecx),
                   "=d" (out->edx)
                 : "a" (leaf), "c" (subleaf));
#endif
}

/* Open-code the RDRAND instruction to support older assemblers */
#ifdef __x86_64__
# define RDRAND ".byte 0x48,0x0f,0xc7,0xf0" /* rdrand %rax */
typedef unsigned long long rdrand_t;
#else
# define RDRAND ".byte 0x0f,0xc7,0xf0"      /* rdrand %eax */
typedef unsigned int rdrand_t;
#endif

static inline bool rdrand(rdrand_t *ptr)
{
    unsigned int ctr = 10;      /* Maximum number of rdrand attempts */

    asm volatile("1:\n"
                 " " RDRAND "\n"
                 " jc 2f\n"
                 " dec %1\n"
                 " jnz 1b\n"
                 "2:"
                 : "=a" (*ptr), "+r" (ctr));

    return ctr > 0;
}

static bool hwrand_rdrand(unsigned char *buf, int bufsize)
{
    rdrand_t tmp;

    while (bufsize >= sizeof(rdrand_t)) {
        if (!rdrand((rdrand_t *)buf))
            return false;

        buf     += sizeof(rdrand_t);
        bufsize -= sizeof(rdrand_t);
    }

    if (!bufsize)
        return true;

    if (!rdrand(&tmp))
        return false;

#ifdef __x86_64__
    if (bufsize & 4) {
        *(unsigned int *)buf = (unsigned int)tmp;
        buf += 4;
        tmp >>= 32;
    }
#endif

    if (bufsize & 2) {
        *(unsigned short *)buf = (unsigned short)tmp;
        buf += 2;
        tmp >>= 16;
    }

    if (bufsize & 1)
        *buf = (unsigned char)tmp;

    return true;
}

hwrand_t init_hwrand(void)
{
    struct cpuid cpu;

    if (!have_cpuid())
        return NULL;

    cpuid(0, 0, &cpu);
    if (cpu.eax < 1)
        return NULL;

    cpuid(1, 0, &cpu);
    if (!(cpu.ecx & (1 << 30)))
        return NULL;

    return hwrand_rdrand;
}

#else /* Not __GNUC__ or not x86 */

hwrand_t init_hwrand(void)
{
    return NULL;
}

#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
