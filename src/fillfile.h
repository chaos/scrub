typedef void (*progress_t) (void *arg, double completed);
typedef void (*refill_t) (unsigned char *mem, int memsize);

off_t fillfile(char *path, off_t filesize, unsigned char *mem, int memsize,
        progress_t progress, void *arg, refill_t refill,
        bool sparse, bool creat);
off_t checkfile(char *path, off_t filesize, unsigned char *mem, int memsize,
        progress_t progress, void *arg, bool sparse);
void  disable_threads(void);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
