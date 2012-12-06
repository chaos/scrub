#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/stat.h>
#include <sys/param.h> 
#include <sys/resource.h>
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

#define BUFSIZE (4*1024*1024) /* default blocksize */
#define COND_ESCRUB_ERROR(_cond) if (_cond) goto failed;

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
scrub(char *path, off_t size, const sequence_t *seq, bool sparse, 
      bool enospc, bool *isfull)
{
    int i;
    prog_t p;
    unsigned char *buf;
    int bufsize = BUFSIZE;
    off_t written, checked;
    scrub_errnum_t errnum = ESCRUB_SUCCESS;

    if (!(buf = alloc_buffer(bufsize))) {
        return ESCRUB_NOMEM;
    }
    COND_ESCRUB_ERROR(initrand() < 0);
    for (i = 0; i < seq->len; i++) {
        if (i > 0)
            enospc = false;
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
                if (checked < written) {
                    errnum = ESCRUB_VERIFY;
                    goto finish;
                }
                break;
        }
        if (written < size) {            
            if (isfull)
                *isfull = true;
            size = written;
            COND_ESCRUB_ERROR(i != 0 || size == 0);
        }
    }
    errnum = ESCRUB_SUCCESS;
    goto finish;
failed:
    switch (errnum) {
        case ENOMEM:
            errnum = ESCRUB_NOMEM;
            break;
        case EINVAL:
            errnum = ESCRUB_INVAL;
            break;
#if WITH_PTHREADS
        case ESRCH:
        case EDEADLK:
        case EAGAIN:
        case EPERM:
            errnum = ESCRUB_PTHREAD;
            break;
#endif
        default:
            errnum = ESCRUB_FAILED;
    }
finish:
    free(buf);
    return errnum;
}

int
scrub_write (scrub_ctx_t c, void (*progress_cb)(void *arg, double pct_done),
             void *arg)
{
    char *path;
    off_t size;
    const sequence_t *seq;
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
    
    path = c->path;
    seq = seq_lookup_byindex(c->method);
    ft = filetype(path);
   
    switch (ft) {
        case FILE_NOEXIST:
        case FILE_OTHER:
            c->errnum = ESCRUB_FILETYPE;
            goto error;    
        case FILE_BLOCK:
        case FILE_CHAR:
            if (access(path, R_OK|W_OK) < 0) {
                c->errnum = ESCRUB_PERM;
                goto error;
            }
            if (getsize(path, &size) < 0) {
                c->errnum = ESCRUB_PLATFORM;
                goto error;
            }
            break;
        case FILE_LINK:
        case FILE_REGULAR:
            if (access(path, R_OK|W_OK) < 0) {
                c->errnum = ESCRUB_PERM;
                goto error;
            }
            if (stat(path, &sb) < 0) {
                c->errnum = ESCRUB_PLATFORM;
                goto error;
            }
            if (sb.st_size == 0) {
                c->errnum = ESCRUB_EMPTYFILE;
                goto error;
            }
            size = blkalign(sb.st_size, sb.st_blksize, UP);
            break;
    }

    c->errnum = scrub (path, size, seq, false, false, NULL);
    return 0;
error:
    return -1;
}

int
scrub_write_free (scrub_ctx_t c, 
                  void (*progress_cb)(void *arg, double pct_done), void *arg)
{
    char *dirpath;
    off_t size;
    const sequence_t *seq;
    filetype_t ft;
    char path[MAXPATHLEN];
    int fileno = 0;
    struct stat sb;
    bool isfull = false;
    struct rlimit r;
    scrub_errnum_t se;

    if (!c->path) {
        c->errnum = ESCRUB_NOPATH;
        goto error;
    }
    if (!c->method) {
        c->errnum = ESCRUB_INVAL;
        goto error;
    }
    
    dirpath = c->path;
    seq = seq_lookup_byindex(c->method);
    c->errnum = ESCRUB_SUCCESS;
    
    if (filetype(dirpath) != FILE_NOEXIST) {
        c->errnum = ESCRUB_DIREXISTS;
        goto error; 
    }
    if (mkdir(dirpath, 0755) < 0) {
        c->errnum = ESCRUB_PLATFORM;
        goto error;
    }
    if (stat(dirpath, &sb) < 0) {
        c->errnum = ESCRUB_PLATFORM;
        goto error;
    }
    if (getuid == 0) {
        // 'set_rlimit_fsize' in mainline
        r.rlim_cur = r.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_FSIZE, &r) < 0) {
            c->errnum = ESCRUB_PLATFORM;
            goto error;
        }
    }
    // 'get_rlimit_fsize' in mainline
    if (getrlimit(RLIMIT_FSIZE, &r) < 0) {
        c->errnum = ESCRUB_PLATFORM;
        goto error;
    }
    size = r.rlim_cur;
    if (size == 0 || size == RLIM_INFINITY)
        size = 1024*1024*1024;
    size = blkalign(size, sb.st_blksize, DOWN);
    
    do {
        snprintf(path, sizeof(path), "%s/scrub.%.3d", dirpath, fileno++);
        se = scrub(path, size, seq, false, true, &isfull);
        if (se != ESCRUB_SUCCESS)
            c->errnum = se;
    } while (!isfull);
    
    while (--fileno >= 0) {
        snprintf(path, sizeof(path), "%s/scrub.%.3d", dirpath, fileno);
        if (unlink(path) < 0)
            c->errnum = (c->errnum == ESCRUB_FAILED) ? 
                         ESCRUB_FAILED : ESCRUB_PLATFORM;
    }
    if (rmdir(dirpath) < 0)
        c->errnum = ESCRUB_PLATFORM;
    
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
    { ESCRUB_EMPTYFILE,   "Path refers to an empty file" },
    { ESCRUB_INVAL,       "Invalid argument" },
    { ESCRUB_FILETYPE,    "Path refers to unsupported file type"},
    { ESCRUB_PERM,        "Permission denied"},
    { ESCRUB_DIREXISTS,   "Directory already exists"},
    { ESCRUB_VERIFY,      "Verification error"},
    { ESCRUB_PLATFORM,    "Platform error"},
    { ESCRUB_PTHREAD,     "Error related to pthreads"},
    { ESCRUB_FAILED,      "Scrub failed"},
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
