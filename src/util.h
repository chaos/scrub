#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else
typedef enum { false, true } bool;
#endif

typedef enum {
    FILE_NOEXIST,
    FILE_REGULAR,
    FILE_CHAR,
    FILE_BLOCK,
    FILE_LINK,
    FILE_OTHER,
} filetype_t;

typedef enum { UP, DOWN } round_t;

int         read_all(int fd, unsigned char *buf, int count);
int         write_all(int fd, const unsigned char *buf, int count);
filetype_t  filetype(char *path);
off_t       blkalign(off_t offset, int blocksize, round_t rtype);
void *      alloc_buffer(int bufsize);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
