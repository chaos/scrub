typedef enum {
	ESCRUB_SUCCESS = 0,
	ESCRUB_NOMEM = 1,
	ESCRUB_NOPATH = 2,
	ESCRUB_SIGCHECK = 3,
	ESCRUB_ISDIR = 4,
	ESCRUB_NOENT = 5,
	ESCRUB_INVAL = 6,
	ESCRUB_FILETYPE = 7,
	ESCRUB_PERM = 8,
} scrub_errnum_t;

typedef enum {
	SCRUB_ATTR_METHOD = 0,
} scrub_attr_t;

typedef struct scrub_ctx_struct *scrub_ctx_t;

int   scrub_init (scrub_ctx_t *cp);

void  scrub_fini (scrub_ctx_t c);

int   scrub_attr_set (scrub_ctx_t c, scrub_attr_t attr, int val);

int   scrub_attr_get (scrub_ctx_t c, scrub_attr_t attr, int *val);

int   scrub_path_set (scrub_ctx_t c, const char *path);

int   scrub_path_get (scrub_ctx_t c, const char **pathp);

int   scrub_write (scrub_ctx_t c,
                   void (*progress_cb)(void *arg, double pct_done),
		   void *arg);

int   scrub_check_sig (scrub_ctx_t c, int *status);

const char *scrub_strerror (scrub_ctx_t c);

int scrub_errno (scrub_ctx_t c);

int   scrub_methods_get (scrub_ctx_t c, char ***methods, int *len);
