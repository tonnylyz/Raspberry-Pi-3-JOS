#ifndef OSLABPI_FSIPC_H
#define OSLABPI_FSIPC_H

#include <lib.h>
#include <fd.h>

int fsipc_open(const char *path, u_long omode, struct Fd *fd);

int fsipc_map(u_long fileid, u_long offset, u_long dstva);

int fsipc_set_size(u_long fileid, u_long size);

int fsipc_close(u_long fileid);

int fsipc_dirty(u_long fileid, u_long offset);

int fsipc_remove(const char *path);

int fsipc_sync(void);

#endif //OSLABPI_FSIPC_H
