typedef struct scrub_ctx_struct *scrub_ctx_t;

int   scrub_init (const char *path, scrub_ctx_t *cp);

void  scrub_fini (scrub_ctx_t c);

int   scrub_attr_set (scrub_ctx_t c, int attr, int val);

int   scrub_attr_get (scrub_ctx_t c, int attr, int *val);

int   scrub_write (scrub_ctx_t c,
                   void (*progress_cb)(void *arg, double pct_done),
		   void *arg);

int   scrub_methods_get (scrub_ctx_t c, char ***methods, int *len);

int   scrub_check_sig (scrub_ctx_t c);

const char *scrub_strerror (scrub_ctx_t c);

int scrub_errno (scrub_ctx_t c);
