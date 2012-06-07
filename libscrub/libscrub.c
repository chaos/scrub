#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

struct scrub_ctx_struct {
    char *path;
    scrub_errnum_t errnum;
    char **methods;
    int method;
};

int
scrub_init (scrub_ctx_t *cp)
{
    scrub_ctx_t c;

    if (!(c = malloc (sizeof *c)))
        goto nomem;
    memset (c, 0, sizeof (*c));
    *cp = c;
    return 0;
nomem:
    errno = ENOMEM;
    return -1;
}

void
scrub_fini (scrub_ctx_t c)
{
    int i;
    const int len = seq_count();

    if (c->path)
        free (c->path);
    if (c->methods) {
        for (i = 0; i < len; i++)
            free (c->methods[i]);
        free (c->methods);
    }
    free (c);
}

int
scrub_path_set (scrub_ctx_t c, const char *path)
{
    if (c->path)
        free (c->path);
    if (!(c->path = strdup (path)))
        goto nomem;
    return 0;
nomem:
    c->errnum = ESCRUB_NOMEM;
    return -1;
}

int
scrub_path_get (scrub_ctx_t c, const char **pathp)
{
    if (!c->path) {
        c->errnum = ESCRUB_NOPATH;
        goto error;
    }
    *pathp = c->path;
    return 0;
error:
    return -1;
}

int
scrub_attr_set (scrub_ctx_t c, scrub_attr_t attr, int val)
{
    switch (attr) {
        case SCRUB_ATTR_METHOD:
            if (val < 0 || val >= seq_count())
                goto einval;
            c->method = val;
            break;
        default:
            goto einval;        
    }
    return 0;
einval:
    c->errnum = ESCRUB_INVAL;
    return -1;
}

int
scrub_attr_get (scrub_ctx_t c, scrub_attr_t attr, int *val)
{
    switch (attr) {
        case SCRUB_ATTR_METHOD:
            *val = c->method;
            break;
        default:
            goto einval;        
    }
    return 0;
einval:
    c->errnum = ESCRUB_INVAL;
    return -1;
}

int
scrub_write (scrub_ctx_t c, void (*progress_cb)(void *arg, double pct_done),
             void *arg)
{
    if (!c->path) {
        c->errnum = ESCRUB_NOPATH;
        goto error;
    }
    return 0;
error:
    return -1;
}

int
scrub_methods_get (scrub_ctx_t c, char ***mp, int *lp)
{
    const int len = seq_count();
    const sequence_t *sp;
    char buf[80];
    int i;

    if (!c->methods) {
        if (!(c->methods = malloc (len * sizeof(char *))))
            goto nomem;
        memset (c->methods, 0, len * sizeof(char *));
        for (i = 0; i < len; i++) {
            sp = seq_lookup_byindex (i);
            seq2str (sp, buf, sizeof(buf));
            if (!(c->methods[i] = strdup (buf)))
                goto nomem;
        }
    }
    *mp = c->methods;
    *lp = len;
    return 0;
nomem:
    if (c->methods) {
        for (i = 0; i < len; i++)
            if (c->methods[i])
                free(c->methods[i]);
        free (c->methods);
        c->methods = NULL;
    }
    c->errnum = ESCRUB_NOMEM;
    return -1;
}

int
scrub_check_sig (scrub_ctx_t c, int *sp)
{
    bool status;

    if (!c->path) {
        c->errnum = ESCRUB_NOPATH;
        goto error;
    }
    if (checksig(c->path, &status) < 0) {
        switch (errno) {
            case EISDIR:
                c->errnum = ESCRUB_ISDIR;
                break;
            case ENOENT:
                c->errnum = ESCRUB_NOENT;
                break;
            default:
                c->errnum = ESCRUB_SIGCHECK;
                break;
        }
        goto error;
    }
    *sp = status ? 1 : 0;
    return 0;
error:
    return -1;
}

struct etab {
    scrub_errnum_t e;
    const char *s;
};

static struct etab etable[] = {
    { ESCRUB_SUCCESS,     "No error" },
    { ESCRUB_NOMEM,       "Out of memory" },
    { ESCRUB_NOPATH,      "Path is not set" },
    { ESCRUB_SIGCHECK,    "Error checking for scrub signature" },
    { ESCRUB_SIGCHECK,    "Error checking for scrub signature" },
    { ESCRUB_ISDIR,       "Path refers to a directory" },
    { ESCRUB_NOENT,       "No such file or directory" },
    { ESCRUB_INVAL,       "Invalid argument" },
    { ESCRUB_FILETYPE,    "Path refers to unsupported file type"},
    { ESCRUB_PERM,        "Permission denied"},
    { -1, NULL },
};

const char *
scrub_strerror (scrub_ctx_t c)
{
    int i;

    for (i = 0; etable[i].e != -1; i++)
        if (c->errnum == etable[i].e)
            return etable[i].s;
    return "Unknown error";
}

scrub_errnum_t 
scrub_errnum (scrub_ctx_t c)
{
    return c->errnum;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
