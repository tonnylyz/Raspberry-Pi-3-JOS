#include <fd.h>
#include <file.h>

#define MAXFD 32
#define FILEBASE 0x60000000
#define FDTABLE (FILEBASE-PDMAP)

#define INDEX2FD(i)    (FDTABLE+(i)*BY2PG)
#define INDEX2DATA(i)    (FILEBASE+(i)*PDMAP)

static struct Dev *devtab[] = {
        &devfile,
        0,//&devcons,
        0,//&devpipe,
        0
};

int dev_lookup(int dev_id, struct Dev **dev) {
    int i;
    for (i = 0; devtab[i]; i++)
        if (devtab[i]->dev_id == dev_id) {
            *dev = devtab[i];
            return 0;
        }
    writef("[%08x] unknown device type %d\n", env->env_id, dev_id);
    return -E_INVAL;
}

int fd_alloc(struct Fd **fd) {
    u_int va;
    u_int fdno;
    for (fdno = 0; fdno < MAXFD - 1; fdno++) {
        va = INDEX2FD(fdno);
        u_int pte = syscall_pgtable_entry((u_int)va);
        if (pte == 0) {
            *fd = (struct Fd *) (u_long)va;
            return 0;
        }
    }
    return -E_MAX_OPEN;
}

void fd_close(struct Fd *fd) {
    syscall_mem_unmap(0, (u_long) fd);
}

int fd_lookup(int fdnum, struct Fd **fd) {
    u_int va;
    if (fdnum >= MAXFD) {
        writef("fdnum >= MAXFD\n");
        return -E_INVAL;
    }
    va = INDEX2FD(fdnum);
    u_long pte = syscall_pgtable_entry((u_long)va);
    if (pte != 0) {
        *fd = (struct Fd *) (u_long)va;
        return 0;
    }
    return -E_INVAL;
}

u_int fd2data(struct Fd *fd) {
    return INDEX2DATA(fd2num(fd));
}

int fd2num(struct Fd *fd) {
    return ((u_long) fd - FDTABLE) / BY2PG;
}

u_int num2fd(int fd) {
    return fd * BY2PG + FDTABLE;
}

int close(int fdnum) {
    int r;
    struct Dev *dev;
    struct Fd *fd;
    if ((r = fd_lookup(fdnum, &fd)) < 0
        || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0) {
        return r;
    }
    r = (*dev->dev_close)(fd);
    fd_close(fd);
    return r;
}

void close_all(void) {
    int i;
    for (i = 0; i < MAXFD; i++) {
        close(i);
    }
}

int dup(int oldfdnum, int newfdnum) {
    user_panic("dup not implemented!");
}

int read(int fdnum, void *buf, u_int n) {
    int r;
    struct Dev *dev;
    struct Fd *fd;
    r = fd_lookup(fdnum, &fd);
    if (r < 0) {
        writef("[ERR] fd.c : read : fd_lookup : %d", r);
        return r;
    }
    r = dev_lookup(fd->fd_dev_id, &dev);
    if (r < 0) {
        writef("[ERR] fd.c : read : dev_lookup : %d", r);
        return r;
    }
    if ((fd->fd_omode & O_ACCMODE) == O_WRONLY) {
        writef("[%08x] write %d -- bad mode\n", env->env_id, fdnum);
        return -E_INVAL;
    }
    r = (*dev->dev_read)(fd, buf, n, fd->fd_offset);
    if (r > 0) {
        fd->fd_offset += r;
    }
    ((char *) buf)[fd->fd_offset % 512] = '\0';
    return r;
}

int readn(int fdnum, void *buf, u_int n) {
    int m, tot;
    for (tot = 0; tot < n; tot += m) {
        m = read(fdnum, (char *) buf + tot, n - tot);
        if (m < 0) {
            return m;
        }
        if (m == 0) {
            break;
        }
    }
    return tot;
}

int write(int fdnum, const void *buf, u_int n) {
    int r;
    struct Dev *dev;
    struct Fd *fd;
    if ((r = fd_lookup(fdnum, &fd)) < 0
        || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0) {
        return r;
    }
    if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
        writef("[%08x] write %d -- bad mode\n", env->env_id, fdnum);
        return -E_INVAL;
    }
    DUMMY;
    r = (*dev->dev_write)(fd, buf, n, fd->fd_offset);
    DUMMY;
    if (r > 0) {
        fd->fd_offset += r;
    }
    return r;
}

int seek(int fdnum, u_int offset) {
    int r;
    struct Fd *fd;
    if ((r = fd_lookup(fdnum, &fd)) < 0) {
        return r;
    }
    fd->fd_offset = offset;
    return 0;
}


int fstat(int fdnum, struct Stat *stat) {
    int r;
    struct Dev *dev;
    struct Fd *fd;
    if ((r = fd_lookup(fdnum, &fd)) < 0
        || (r = dev_lookup(fd->fd_dev_id, &dev)) < 0) {
        return r;
    }
    stat->st_name[0] = 0;
    stat->st_size = 0;
    stat->st_isdir = 0;
    stat->st_dev = dev;
    return (*dev->dev_stat)(fd, stat);
}

int stat(const char *path, struct Stat *stat) {
    int fd, r;
    if ((fd = open(path, O_RDONLY)) < 0) {
        return fd;
    }
    r = fstat(fd, stat);
    close(fd);
    return r;
}

