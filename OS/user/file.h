#ifndef OSLABPI_FILE_H
#define OSLABPI_FILE_H

#include <lib.h>

int open(const char *path, int mode);

int read_map(int fd, u_int offset, void **blk);

int delete(const char *path);

int ftruncate(int fd, u_int size);

int sync(void);

#endif //OSLABPI_FILE_H
