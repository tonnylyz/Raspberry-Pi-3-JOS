#include <fsipc.h>

extern u_char fsipcbuf[BY2PG];

static int fsipc(u_long type, void *fsreq, u_long dstva, u_long *perm) {
    struct Env *ipc_envs = (struct Env *) ((u_long)UENVS);
    u_long whom;
    ipc_send(ipc_envs[1].env_id, type, (u_long) fsreq, PTE_V | ATTRIB_AP_RW_ALL);
    return ipc_recv(&whom, dstva, perm);
}

int fsipc_open(const char *path, u_long omode, struct Fd *fd) {
    u_long perm;
    struct Fsreq_open *req;
    req = (struct Fsreq_open *) fsipcbuf;
    if (strlen(path) >= MAXPATHLEN) {
        return -E_BAD_PATH;
    }
    strcpy((char *) req->req_path, path);
    req->req_omode = omode;
    return fsipc(FSREQ_OPEN, req, (u_long) fd, &perm);
}

int fsipc_map(u_long fileid, u_long offset, u_long dstva) {
    int r;
    u_long perm;
    struct Fsreq_map *req;
    req = (struct Fsreq_map *) fsipcbuf;
    req->req_fileid = fileid;
    req->req_offset = offset;
    if ((r = fsipc(FSREQ_MAP, req, dstva, &perm)) < 0) {
        return r;
    }
    /*if ((perm & ~(PTE_R | PTE_LIBRARY)) != (PTE_V)) {
        user_panic("fsipc_map: unexpected permissions %08x for dstva %08x", perm,
                   dstva);
    }*/
    return 0;
}

int fsipc_set_size(u_long fileid, u_long size) {
    struct Fsreq_set_size *req;
    req = (struct Fsreq_set_size *) fsipcbuf;
    req->req_fileid = fileid;
    req->req_size = size;
    return fsipc(FSREQ_SET_SIZE, req, 0, 0);
}

int fsipc_close(u_long fileid) {
    struct Fsreq_close *req;
    req = (struct Fsreq_close *) fsipcbuf;
    req->req_fileid = fileid;
    return fsipc(FSREQ_CLOSE, req, 0, 0);
}

int fsipc_dirty(u_long fileid, u_long offset) {
    struct Fsreq_dirty *req;
    req = (struct Fsreq_dirty *) fsipcbuf;
    req->req_fileid = fileid;
    req->req_offset = offset;
    return fsipc(FSREQ_DIRTY, req, 0, 0);
}

int fsipc_remove(const char *path) {
    struct Fsreq_remove *req;
    req = (struct Fsreq_remove *) fsipcbuf;
    if (strlen(path) >= MAXNAMELEN) return -1;
    strcpy((char *)req->req_path, path);
    return fsipc(FSREQ_REMOVE, req, 0, 0);
}

int fsipc_sync(void) {
    return fsipc(FSREQ_SYNC, fsipcbuf, 0, 0);
}

