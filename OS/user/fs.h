#ifndef _FS_H_
#define _FS_H_

#include <lib.h>

int file_open(char *path, struct File **pfile);

int file_get_block(struct File *f, u_int blockno, void **pblk);

int file_set_size(struct File *f, u_int newsize);

void file_close(struct File *f);

int file_remove(char *path);

void fs_init(void);

int file_dirty(struct File *f, u_int offset);

void fs_sync(void);

void file_flush(struct File *);

extern u_int *bitmap;

int map_block(u_int);

int alloc_block(void);

#endif // _FS_H_
