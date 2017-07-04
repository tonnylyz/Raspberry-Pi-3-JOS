#include <fs.h>

static void dummy(u_long a) {
    a++;
    return;
}

struct Super *super;

u_int nbitmap;
u_int *bitmap;

int block_is_free(u_int);

u_int diskaddr(u_int blockno) {
    if (super && blockno >= super->s_nblocks)
        user_panic("[ERR] diskaddr : blockno greater than disk's nblocks");
    return DISKMAP + blockno * BY2BLK;
}

u_int va_is_mapped(u_int va) {
    return syscall_pgtable_entry(va) != 0;
}

u_int block_is_mapped(u_int blockno) {
    u_int va = diskaddr(blockno);
    if (va_is_mapped(va)) {
        return va;
    }
    return 0;
}

u_int va_is_dirty(u_int va) {
    return 0; // NEVER DIRTY
}

u_int block_is_dirty(u_int blockno) {
    u_int va = diskaddr(blockno);
    return va_is_mapped(va) && va_is_dirty(va);
}

int map_block(u_int blockno) {
    if (block_is_mapped(blockno)) return 0;
    return syscall_mem_alloc(0, diskaddr(blockno), ATTRIB_AP_RW_ALL);
}

void unmap_block(u_int blockno) {
    if (!block_is_mapped(blockno)) return;
    if (!block_is_free(blockno) && block_is_dirty(blockno)) {
        writef("[ERR] unmap_block GAH!");
        //write_block(blockno);
    }
    syscall_mem_unmap(0, diskaddr(blockno));
    user_assert(!block_is_mapped(blockno));
}

int read_block(u_int blockno, void **blk, u_int *isnew) {
    u_int va;
    if (super && blockno >= super->s_nblocks) {
        user_panic("reading non-existent block %08x\n", blockno);
    }
    if (bitmap && block_is_free(blockno)) {
        user_panic("reading free block %08x\n", blockno);
    }
    va = diskaddr(blockno);
    if (block_is_mapped(blockno)) {
        if (isnew) {
            *isnew = 0;
        }
    } else {
        if (isnew) {
            *isnew = 1;
        }
        syscall_emmc_read(blockno * SECT2BLK, va);
    }
    if (blk) {
        DUMMY;
        *blk = (void *) (u_long)va;
    }
    return 0;
}

void write_block(u_int blockno) {
    // TODO: Not support write!
    return;
    u_int va;
    if (!block_is_mapped(blockno)) {
        user_panic("write unmapped block %08x", blockno);
    }
    va = diskaddr(blockno);
    //ide_write(0, blockno * SECT2BLK, (void *) va, SECT2BLK);
    // TODO: GO EMMC WRITE?
    syscall_mem_map(0, va, 0, va, ATTRIB_AP_RW_ALL);
}

int block_is_free(u_int blockno) {
    if (super == 0 || blockno >= super->s_nblocks) {
        return 0;
    }
    if (bitmap[blockno / 32] & (1 << (blockno % 32))) {
        return 1;
    }
    return 0;
}

void free_block(u_int blockno) {
    if (blockno == 0)
        user_panic("[ERR] free_block : blockno == 0");
    bitmap[blockno / 32] |= (1 << (blockno % 32));
}

int alloc_block_num(void) {
    int blockno;
    for (blockno = 3; blockno < super->s_nblocks; blockno++) {
        if (bitmap[blockno / 32] & (1 << (blockno % 32))) {
            bitmap[blockno / 32] &= ~(1 << (blockno % 32));
            write_block(blockno / BIT2BLK);
            return blockno;
        }
    }
    return -E_NO_DISK;
}

int alloc_block(void) {
    int r, bno;
    if ((r = alloc_block_num()) < 0) {
        return r;
    }
    bno = r;
    if ((r = map_block(bno)) < 0) {
        free_block(bno);
        return r;
    }
    return bno;
}

void read_super(void) {
    int r;
    void *blk = 0;
    if ((r = read_block(1, &blk, 0)) < 0) {
        user_panic("cannot read superblock: %e", r);
    }
    super = blk;
    if ((u_int)(super->s_magic) != (u_int)FS_MAGIC) {
        user_panic("bad file system magic number %x %x", super->s_magic, FS_MAGIC);
    }
    if (super->s_nblocks > DISKMAX / BY2BLK) {
        user_panic("file system is too large");
    }
    writef("superblock is good\n");
}

void read_bitmap(void) {
    u_int i;
    void *blk = 0;
    nbitmap = super->s_nblocks / BIT2BLK + 1;
    for (i = 0; i < nbitmap; i++) {
        read_block(i + 2, &blk, 0);
    }
    bitmap = (u_long *)(u_long)diskaddr(2);
    user_assert(!block_is_free(0));
    user_assert(!block_is_free(1));
    for (i = 0; i < nbitmap; i++) {
        user_assert(!block_is_free(i + 2));
    }
    writef("read_bitmap is good\n");
}

void fs_init(void) {
    read_super();
    read_bitmap();
}

int file_block_walk(struct File *f, u_int filebno, u_int **ppdiskbno, u_int alloc) {
    int r;
    u_int *ptr;
    void *blk;
    if (filebno < NDIRECT) {
        ptr = &f->f_direct[filebno];
    } else if (filebno < NINDIRECT) {
        if (f->f_indirect == 0) {
            if (alloc == 0) {
                return -E_NOT_FOUND;
            }
            if ((r = alloc_block()) < 0) {
                return r;
            }
            f->f_indirect = r;
        }
        if ((r = read_block(f->f_indirect, &blk, 0)) < 0) {
            return r;
        }
        ptr = (u_int *) blk + filebno;
    } else {
        return -E_INVAL;
    }
    *ppdiskbno = ptr;
    return 0;
}

int file_map_block(struct File *f, u_int filebno, u_int *diskbno, u_int alloc) {
    int r;
    u_int *ptr;
    if ((r = file_block_walk(f, filebno, &ptr, alloc)) < 0) {
        return r;
    }
    if (*ptr == 0) {
        if (alloc == 0) {
            return -E_NOT_FOUND;
        }
        if ((r = alloc_block()) < 0) {
            return r;
        }
        *ptr = r;
    }
    *diskbno = *ptr;
    return 0;
}

int file_clear_block(struct File *f, u_int filebno) {
    int r;
    u_int *ptr;
    if ((r = file_block_walk(f, filebno, &ptr, 0)) < 0) {
        return r;
    }
    if (*ptr) {
        free_block(*ptr);
        *ptr = 0;
    }
    return 0;
}

int file_get_block(struct File *f, u_int filebno, void **blk) {
    int r;
    u_int diskbno;
    u_int isnew;
    if ((r = file_map_block(f, filebno, &diskbno, 1)) < 0) {
        return r;
    }
    if ((r = read_block(diskbno, blk, &isnew)) < 0) {
        return r;
    }
    return 0;
}

int file_dirty(struct File *f, u_int offset) {
    int r;
    void *blk;
    if ((r = file_get_block(f, offset / BY2BLK, &blk)) < 0) {
        return r;
    }
    *(volatile char *) blk = *(volatile char *) blk;
    return 0;
}

int dir_lookup(struct File *dir, char *name, struct File **file) {
    int r;
    u_int i, j, nblock;
    void *blk;
    struct File *f;
    nblock = dir->f_size / BY2BLK;
    for (i = 0; i < nblock; i++) {
        r = file_get_block(dir, i, &blk);
        if (r < 0) {
            return r;
        }
        f = blk;
        for (j = 0; j < FILE2BLK; j++) {
            if (strcmp((char *)f[j].f_name, name) == 0) {
                *file = &f[j];
                f[j].f_dir = dir;
                return 0;
            }
        }
    }
    return -E_NOT_FOUND;
}

int dir_alloc_file(struct File *dir, struct File **file) {
    int r;
    u_int nblock, i, j;
    void *blk;
    struct File *f;
    nblock = dir->f_size / BY2BLK;
    for (i = 0; i < nblock; i++) {
        if ((r = file_get_block(dir, i, &blk)) < 0) {
            return r;
        }
        f = blk;
        for (j = 0; j < FILE2BLK; j++) {
            if (f[j].f_name[0] == '\0') {
                *file = &f[j];
                return 0;
            }
        }
    }
    dir->f_size += BY2BLK;
    if ((r = file_get_block(dir, i, &blk)) < 0) {
        return r;
    }
    f = blk;
    *file = &f[0];
    return 0;
}

char *skip_slash(char *p) {
    while (*p == '/') {
        p++;
    }
    return p;
}

int walk_path(char *path, struct File **pdir, struct File **pfile, char *lastelem) {
    char *p;
    char name[MAXNAMELEN];
    struct File *dir, *file;
    int r;
    path = skip_slash(path);
    file = &super->s_root;
    dir = 0;
    name[0] = 0;
    if (pdir) {
        *pdir = 0;
    }
    *pfile = 0;
    while (*path != '\0') {
        dir = file;
        p = path;
        while (*path != '/' && *path != '\0') {
            path++;
        }
        if (path - p >= MAXNAMELEN) {
            return -E_BAD_PATH;
        }
        user_bcopy(p, name, path - p);
        name[path - p] = '\0';
        path = skip_slash(path);
        if (dir->f_type != FTYPE_DIR) {
            return -E_NOT_FOUND;
        }
        if ((r = dir_lookup(dir, name, &file)) < 0) {
            if (r == -E_NOT_FOUND && *path == '\0') {
                if (pdir) {
                    *pdir = dir;
                }
                if (lastelem) {
                    strcpy(lastelem, name);
                }
                *pfile = 0;
            }
            return r;
        }
    }
    if (pdir) {
        *pdir = dir;
    }
    *pfile = file;
    return 0;
}

int file_open(char *path, struct File **file) {
    return walk_path(path, 0, file, 0);
}

int file_create(char *path, struct File **file) {
    char name[MAXNAMELEN];
    int r;
    struct File *dir, *f;
    if ((r = walk_path(path, &dir, &f, name)) == 0) {
        return -E_FILE_EXISTS;
    }
    if (r != -E_NOT_FOUND || dir == 0) {
        return r;
    }
    if (dir_alloc_file(dir, &f) < 0) {
        return r;
    }
    strcpy((char *) f->f_name, name);
    *file = f;
    return 0;
}

void file_truncate(struct File *f, u_int newsize) {
    u_int bno, old_nblocks, new_nblocks;
    old_nblocks = f->f_size / BY2BLK + 1;
    new_nblocks = newsize / BY2BLK + 1;
    if (newsize == 0) {
        new_nblocks = 0;
    }
    if (new_nblocks <= NDIRECT) {
        f->f_indirect = 0;
        for (bno = new_nblocks; bno < old_nblocks; bno++) {
            file_clear_block(f, bno);
        }
    } else {
        for (bno = new_nblocks; bno < old_nblocks; bno++) {
            file_clear_block(f, bno);
        }
    }
    f->f_size = newsize;
}

int file_set_size(struct File *f, u_int newsize) {
    if (f->f_size > newsize) {
        file_truncate(f, newsize);
    }
    f->f_size = newsize;
    if (f->f_dir) {
        file_flush(f->f_dir);
    }
    return 0;
}

void file_flush(struct File *f) {
    u_int nblocks;
    u_int bno;
    u_int diskno;
    int r;
    nblocks = f->f_size / BY2BLK + 1;
    for (bno = 0; bno < nblocks; bno++) {
        if ((r = file_map_block(f, bno, &diskno, 0)) < 0) {
            continue;
        }
        if (block_is_dirty(diskno)) {
            write_block(diskno);
        }
    }
}

void fs_sync(void) {
    int i;
    for (i = 0; i < super->s_nblocks; i++) {
        if (block_is_dirty(i)) {
            write_block(i);
        }
    }
}

void file_close(struct File *f) {
    file_flush(f);
    if (f->f_dir) {
        file_flush(f->f_dir);
    }
}

int file_remove(char *path) {
    int r;
    struct File *f;
    if ((r = walk_path(path, 0, &f, 0)) < 0) {
        return r;
    }
    file_truncate(f, 0);
    f->f_name[0] = '\0';
    file_flush(f);
    if (f->f_dir) {
        file_flush(f->f_dir);
    }
    return 0;
}
