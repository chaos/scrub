#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

//TODO: REMOVE
#include <stdio.h>

#define BUFSIZE (4*1024*1024) /* default blocksize */
#define COND_ESCRUB_ERROR(_cond) if (_cond) return ESCRUB_PLATFORM

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

static scrub_errnum_t
scrub(char *path, off_t size, const sequence_t *seq, unsigned char *buf)
{
    int i;
    prog_t p;
    int bufsize = BUFSIZE;
    bool sparse = false, enospc = false;
    off_t written, checked;

    COND_ESCRUB_ERROR(initrand() < 0);
    for (i = 0; i < seq->len; i++) {
        switch (seq->pat[i].ptype) {
            case PAT_RANDOM:
                COND_ESCRUB_ERROR(churnrand() < 0);
                progress_create(&p, 50);
                
                written = fillfile(path, size, buf, bufsize,
                                   (progress_t) progress_update, p,
                                   (refill_t) genrand, sparse, enospc);
                
                progress_destroy(p);
                COND_ESCRUB_ERROR(written == (off_t) -1);
                break;
            case PAT_NORMAL:
                progress_create(&p, 50);
                
                memset_pat(buf, seq->pat[i], bufsize);
                written = fillfile(path, size, buf, bufsize,
                                   (progress_t) progress_update, p,
                                   NULL, sparse, enospc);
                
                progress_destroy(p);                
                COND_ESCRUB_ERROR(written == (off_t) -1);
                break;
            case PAT_VERIFY:
                progress_create(&p, 50);
                
                memset_pat(buf, seq->pat[i], bufsize);
                written = fillfile(path, size, buf, bufsize,
                                   (progress_t) progress_update, p,
                                   NULL, sparse, enospc);
                
                progress_destroy(p);
                COND_ESCRUB_ERROR(written == (off_t) -1);
                progress_create(&p, 50);
                
                checked = checkfile(path, written, buf, bufsize,
                                    (progress_t) progress_update, p, sparse);
                
                progress_destroy(p);
                COND_ESCRUB_ERROR(checked == (off_t) -1);
                COND_ESCRUB_ERROR(checked < written);
                break;
        }
        if (written < size) {            
            size = written;
            COND_ESCRUB_ERROR(i != 0 || size == 0);
        }
    }
    return ESCRUB_SUCCESS;
}

int
scrub_write (scrub_ctx_t c, void (*progress_cb)(void *arg, double pct_done),
             void *arg)
{
    char *path;
    off_t size;
    const sequence_t *seq;
    unsigned char *buf;
    filetype_t ft;
    struct stat sb;

    if (!c->path) {
        c->errnum = ESCRUB_NOPATH;
        goto error;
    }
    if (!c->method) {
        c->errnum = ESCRUB_INVAL;
        goto error;
    }
    if (!(buf = alloc_buffer(BUFSIZE))) {
        c->errnum = ESCRUB_NOMEM;
        goto error;
    }
    
    path = c->path;
    seq = seq_lookup_byindex(c->method);
    ft = filetype(path);
   
    switch (ft) {
        case FILE_NOEXIST:
        case FILE_OTHER:
            c->errnum = ESCRUB_FILETYPE;
            goto finish;    
        case FILE_BLOCK:
        case FILE_CHAR:
            if (access(path, R_OK|W_OK) < 0) {
                c->errnum = ESCRUB_PERM;
                goto finish;
            }
            if (getsize(path, &size) < 0) {
                c->errnum = ESCRUB_PLATFORM;
                goto finish;
            }
            break;
        case FILE_LINK:
        case FILE_REGULAR:
            if (access(path, R_OK|W_OK) < 0) {
                c->errnum = ESCRUB_PERM;
                goto finish;
            }
            if (stat(path, &sb) < 0) {
                c->errnum = ESCRUB_PLATFORM;
                goto finish;
            }
            if (sb.st_size == 0) {
                // TODO: new error for this?
                c->errnum = ESCRUB_SUCCESS;
                goto finish;
            }
            size = blkalign(sb.st_size, sb.st_blksize, UP);
            break;
    }

    c->errnum = scrub (path, size, seq, buf);

finish:
    free(buf);
    if (!c->errnum || c->errnum != ESCRUB_SUCCESS)
        goto error;
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
    { ESCRUB_ISDIR,       "Path refers to a directory" },
    { ESCRUB_NOENT,       "No such file or directory" },
    { ESCRUB_INVAL,       "Invalid argument" },
    { ESCRUB_FILETYPE,    "Path refers to unsupported file type"},
    { ESCRUB_PERM,        "Permission denied"},
    // TODO: break out errors?
    { ESCRUB_PLATFORM,    "Platform error"},
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
