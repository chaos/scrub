typedef struct prog_struct *prog_t;

void progress_create(prog_t *ctx, int width);
void progress_destroy(prog_t ctx);
void progress_update(prog_t ctx, double complete);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
