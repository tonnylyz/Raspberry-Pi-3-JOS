#ifndef _USER_FD_H_
#define _USER_FD_H_

#include <lib.h>

struct Fd {
    u_int fd_dev_id;
    u_int fd_offset;
    u_int fd_omode;
};
struct Dev;
struct Stat {
    char st_name[MAXNAMELEN];
    u_int st_size;
    u_int st_isdir;
    struct Dev *st_dev;
};


struct Dev {
    int dev_id;
    char *dev_name;

    int (*dev_read)(struct Fd *, void *, u_int, u_int);

    int (*dev_write)(struct Fd *, const void *, u_int, u_int);

    int (*dev_close)(struct Fd *);

    int (*dev_stat)(struct Fd *, struct Stat *);

    int (*dev_seek)(struct Fd *, u_int);
};


struct Filefd {
    struct Fd f_fd;
    u_int f_fileid;
    struct File f_file;
};

int fd_alloc(struct Fd **fd);

int fd_lookup(int fdnum, struct Fd **fd);

u_int fd2data(struct Fd *);

int fd2num(struct Fd *);

int dev_lookup(int dev_id, struct Dev **dev);

u_int num2fd(int fd);

extern struct Dev devcons;
extern struct Dev devfile;
extern struct Dev devpipe;

// fd.c
int close(int fd);

int read(int fd, void *buf, u_int nbytes);

int write(int fd, const void *buf, u_int nbytes);

int seek(int fd, u_int offset);

void close_all(void);

int readn(int fd, void *buf, u_int nbytes);

int dup(int oldfd, int newfd);

int fstat(int fdnum, struct Stat *stat);

int stat(const char *path, struct Stat *);

#endif
