#include <file.h>
#include <lib.h>
#include <fd.h>
#include <fsipc.h>

static int file_close(struct Fd *fd);

static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset);

static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset);

static int file_stat(struct Fd *fd, struct Stat *stat);

struct Dev devfile = {
        .dev_id =    'f',
        .dev_name =    "file",
        .dev_read =    file_read,
        .dev_write =    file_write,
        .dev_close =    file_close,
        .dev_stat =    file_stat,
};

int open(const char *path, int mode) {
    struct Fd *fd;
    struct Filefd *ffd;
    int r;
    u_long va;
    u_long i;
    r = fd_alloc(&fd);
    if (r < 0) {
        return r;
    }
    r = fsipc_open(path, mode, fd);
    if (r < 0) {
        return r;
    }
    va = (u_long)fd2data(fd);
    ffd = (struct Filefd *) fd;
    for (i = 0; i < ffd->f_file.f_size; i += BY2PG) {
        //writef("map %x %x %x\n", (u_long)(ffd->f_fileid) & 0xFFFFFFFF, i, va + i);
        r = fsipc_map((u_long)(ffd->f_fileid) & 0xFFFFFFFF, i, va + i);
        if (r < 0) {
            writef("[ERR] file.c : open() : fsipc_map \n");
            return r;
        }
    }
    //writef("file.c open done %d\n");
    return fd2num(fd);
}

int file_close(struct Fd *fd) {
    int r;
    struct Filefd *ffd;
    u_int va, size, fileid;
    u_int i;
    ffd = (struct Filefd *) fd;
    fileid = ffd->f_fileid;
    size = ffd->f_file.f_size;
    va = fd2data(fd);
    for (i = 0; i < size; i += BY2PG) {
        fsipc_dirty(fileid, i);
    }
    if ((r = fsipc_close(fileid)) < 0) {
        writef("cannot close the file\n");
        return r;
    }
    if (size == 0) {
        return 0;
    }
    for (i = 0; i < size; i += BY2PG) {
        if ((r = syscall_mem_unmap(0, va + i)) < 0) {
            writef("cannont unmap the file.\n");
            return r;
        }
    }
    return 0;
}

static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset) {
    u_int size;
    struct Filefd *f;
    f = (struct Filefd *) fd;
    size = f->f_file.f_size;
    if (offset > size) {
        return 0;
    }
    if (offset + n > size) {
        n = size - offset;
    }
    user_bcopy((void *) (u_long)fd2data(fd) + offset, buf, n);
    return n;
}

int read_map(int fdnum, u_int offset, void **blk) {
    int r;
    u_int va;
    struct Fd *fd;
    if ((r = fd_lookup(fdnum, &fd)) < 0) {
        return r;
    }
    if (fd->fd_dev_id != devfile.dev_id) {
        return -E_INVAL;
    }
    va = fd2data(fd) + offset;
    if (offset >= MAXFILESIZE) {
        return -E_NO_DISK;
    }
    Pte pte = syscall_pgtable_entry(va);
    if (pte == 0) {
        return -E_NO_DISK;
    }
    *blk = (void *) (u_long)va;
    return 0;
}

static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset) {
    int r;
    u_int tot;
    struct Filefd *f;
    f = (struct Filefd *) fd;
    tot = offset + n;
    if (tot > MAXFILESIZE) {
        return -E_NO_DISK;
    }
    if (tot > f->f_file.f_size) {
        if ((r = ftruncate(fd2num(fd), tot)) < 0) {
            return r;
        }
    }
    user_bcopy(buf, (void *) (u_long)fd2data(fd) + offset, n);
    return n;
}

static int file_stat(struct Fd *fd, struct Stat *st) {
    struct Filefd *f;
    f = (struct Filefd *) fd;
    strcpy(st->st_name, (char *) f->f_file.f_name);
    st->st_size = f->f_file.f_size;
    st->st_isdir = f->f_file.f_type == FTYPE_DIR;
    return 0;
}

int ftruncate(int fdnum, u_int size) {
    int i, r;
    struct Fd *fd;
    struct Filefd *f;
    u_int oldsize, va, fileid;
    if (size > MAXFILESIZE) {
        return -E_NO_DISK;
    }
    if ((r = fd_lookup(fdnum, &fd)) < 0) {
        return r;
    }
    if (fd->fd_dev_id != devfile.dev_id) {
        return -E_INVAL;
    }
    f = (struct Filefd *) fd;
    fileid = f->f_fileid;
    oldsize = f->f_file.f_size;
    f->f_file.f_size = size;
    if ((r = fsipc_set_size(fileid, size)) < 0) {
        return r;
    }
    va = fd2data(fd);
    for (i = ROUND(oldsize, BY2PG); i < ROUND(size, BY2PG); i += BY2PG) {
        if ((r = fsipc_map(fileid, i, va + i)) < 0) {
            fsipc_set_size(fileid, oldsize);
            return r;
        }
    }
    for (i = ROUND(size, BY2PG); i < ROUND(oldsize, BY2PG); i += BY2PG)
        if ((r = syscall_mem_unmap(0, va + i)) < 0) {
            user_panic("ftruncate: syscall_mem_unmap %08x: %e", va + i, r);
        }

    return 0;
}

int remove(const char *path) {
    return fsipc_remove(path);
}

int sync(void) {
    return fsipc_sync();
}
