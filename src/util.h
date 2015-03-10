#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else
typedef enum { false, true } bool;
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "pattern.h" 

typedef enum {
    FILE_NOEXIST,
    FILE_REGULAR,
    FILE_CHAR,
    FILE_BLOCK,
    FILE_LINK,
    FILE_OTHER,
} filetype_t;

typedef enum { UP, DOWN } round_t;

struct opt_struct {
    const sequence_t *seq;
    int blocksize;
    off_t devsize;
    char *dirent;
    bool force;
    bool nosig;
    bool remove;
    bool sparse;
    bool nofollow;
    bool nohwrand;
    bool nothreads;
};

int         read_all(int fd, unsigned char *buf, int count);
int         write_all(int fd, const unsigned char *buf, int count);
filetype_t  filetype(char *path);
off_t       blkalign(off_t offset, int blocksize, round_t rtype);
void *      alloc_buffer(int bufsize);
void        read_conf( struct opt_struct *opt );

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
