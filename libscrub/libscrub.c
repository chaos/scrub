#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>

#include "util.h"
#include "filldentry.h"
#include "fillfile.h"
#include "genrand.h"
#include "getsize.h"
#include "hwrand.h"
#include "pattern.h"
#include "progress.h"
#include "sig.h"

#include "scrub.h"

int
scrub_init (const char *path, scrub_ctx_t *cp)
{
    return -1;
}

void
scrub_fini (scrub_ctx_t c)
{
}

int
scrub_attr_set (scrub_ctx_t c, int attr, int val)
{
    return -1;
}


int
scrub_attr_get (scrub_ctx_t c, int attr, int *val)
{
    return -1;
}

int
scrub_write (scrub_ctx_t c, void (*progress_cb)(void *arg, double pct_done),
             void *arg)
{
    return -1;
}

int
scrub_methods_get (scrub_ctx_t c, char ***methods, int *len)
{
    return -1;
}

int
scrub_check_sig (scrub_ctx_t c)
{
    return -1;
}

const char *
scrub_strerror (scrub_ctx_t c)
{
    return "no error";
}

int
scrub_errno (scrub_ctx_t c)
{
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
