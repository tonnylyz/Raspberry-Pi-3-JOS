#include <lib.h>
#include <fd.h>
#include <fs.h>

int strecmp(char *a, char *b) {
	while (*b)
		if (*a++ != *b++) {
			return 1;
		}
	return 0;
}

static char *msg = "This is the NEW message of the day!\r\n\r\n";
static char *diff_msg = "This is a different massage of the day!\r\n\r\n";

void fs_test(void) {
	struct File *f;
	int r;
	void *blk;
	u_long *bits;
	if ((r = syscall_mem_alloc(0, BY2PG, PTE_V | ATTRIB_AP_RW_ALL)) < 0) {
		user_panic("syscall_mem_alloc: %e", r);
	}
	bits = (u_long *) BY2PG;
	user_bcopy(bitmap, bits, BY2PG);
    writef("fs_test got here %d\n", __LINE__);
	if ((r = alloc_block()) < 0) {
		user_panic("alloc_block: %e", r);
	}
	user_assert(bits[r / 32] & (1 << (r % 32)));
	user_assert(!(bitmap[r / 32] & (1 << (r % 32))));
	writef("alloc_block is good\n");
	if ((r = file_open("/not-found", &f)) < 0 && r != -E_NOT_FOUND) {
		user_panic("file_open /not-found: %e", r);
	} else if (r == 0) {
		user_panic("file_open /not-found succeeded!");
	}
	if ((r = file_open("/newmotd", &f)) < 0) {
		user_panic("file_open /newmotd: %d", r);
	}
	writef("file_open is good\n");
	if ((r = file_get_block(f, 0, &blk)) < 0) {
		user_panic("file_get_block: %e", r);
	}

	writef("fs_test got here %d\n", __LINE__);
    writef("blk [%l016x] msg [%l016x]\n", blk, msg);
	if (strecmp(blk, msg) != 0) {
		user_panic("file_get_block returned wrong data");
	}
	writef("file_get_block is good\n");
	*(volatile char *) blk = *(volatile char *) blk;
	file_flush(f);
	writef("file_flush is good\n");
	if ((r = file_set_size(f, 0)) < 0) {
		user_panic("file_set_size: %e", r);
	}
	user_assert(f->f_direct[0] == 0);
	writef("file_truncate is good\n");
	if ((r = file_set_size(f, strlen(diff_msg))) < 0) {
		user_panic("file_set_size 2: %e", r);
	}
	if ((r = file_get_block(f, 0, &blk)) < 0) {
		user_panic("file_get_block 2: %e", r);
	}
	strcpy((char *) blk, diff_msg);
	file_flush(f);
	file_close(f);
	writef("file rewrite is good\n");
}



struct Open {
	struct File *o_file;
	u_int o_fileid;
	int o_mode;
	struct Filefd *o_ff;
};

#define MAXOPEN			1024
#define FILEVA 			0x60000000

struct Open opentab[MAXOPEN] = { { 0, 0, 1 } };

#define REQVA	0x0ffff000

void serve_init(void)
{
	int i;
	u_long va;
	va = FILEVA;
	for (i = 0; i < MAXOPEN; i++) {
		opentab[i].o_fileid = i;
		opentab[i].o_ff = (struct Filefd *)va;
		va += BY2PG;
	}
}

int open_alloc(struct Open **o)
{
	int i, r;
	for (i = 0; i < MAXOPEN; i++) {
		switch (pageref(opentab[i].o_ff)) {
			case 0:
				if ((r = syscall_mem_alloc(0, (u_long)opentab[i].o_ff,
										   PTE_V | ATTRIB_AP_RW_ALL)) < 0) {
					return r;
				}
			case 1:
				opentab[i].o_fileid += MAXOPEN;
				*o = &opentab[i];
				user_bzero((void *)opentab[i].o_ff, BY2PG);
				return (*o)->o_fileid;
		}
	}
	return -E_MAX_OPEN;
}

int open_lookup(u_int envid, u_int fileid, struct Open **po)
{
	struct Open *o;
	o = &opentab[fileid % MAXOPEN];
	if (pageref(o->o_ff) == 1 || o->o_fileid != fileid) {
		return -E_INVAL;
	}
	*po = o;
	return 0;
}

void serve_open(u_int envid, struct Fsreq_open *rq)
{
	writef("serve_open %08x %x 0x%x\n", envid, (u_long)rq->req_path, rq->req_omode);
	u_char path[MAXPATHLEN];
	struct File *f;
	struct Filefd *ff;
	//int fileid;
    writef("#%d\n", __LINE__);

	int r;
	struct Open *o;
	user_bcopy(rq->req_path, path, MAXPATHLEN);
    writef("#%d\n", __LINE__);
	path[MAXPATHLEN - 1] = 0;
	if ((r = open_alloc(&o)) < 0) {
		writef("open_alloc failed: %d, invalid path: %s", r, path);
		ipc_send(envid, r, 0, 0);
	}
    writef("#%d\n", __LINE__);
	if ((r = file_open((char *)path, &f)) < 0) {
		writef("file_open failed: %d, invalid path: %s", r, path);
		ipc_send(envid, r, 0, 0);
	}
    writef("#%d\n", __LINE__);
	o->o_file = f;
	ff = (struct Filefd *)o->o_ff;
	ff->f_file = *f;
	ff->f_fileid = o->o_fileid;
	o->o_mode = rq->req_omode;
	ff->f_fd.fd_omode = o->o_mode;
	ff->f_fd.fd_dev_id = devfile.dev_id;
    writef("#%d\n", __LINE__);
	ipc_send(envid, 0, (u_long)o->o_ff, PTE_V | ATTRIB_AP_RW_ALL);
}

void serve_map(u_int envid, struct Fsreq_map *rq)
{
    writef("serve_map %08x %x\n", envid, (u_long)rq->req_fileid);
	struct Open *pOpen;
	u_int filebno;
	void *blk;
	int r;
    writef("#%d\n", __LINE__);
	if ((r = open_lookup(envid, rq->req_fileid, &pOpen)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}
    writef("#%d\n", __LINE__);
	filebno = rq->req_offset / BY2BLK;
	if ((r = file_get_block(pOpen->o_file, filebno, &blk)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}
    writef("#%d\n", __LINE__);
	ipc_send(envid, 0, (u_long)blk, PTE_V | ATTRIB_AP_RW_ALL);
}

void serve_set_size(u_int envid, struct Fsreq_set_size *rq)
{
	struct Open *pOpen;
	int r;
	if ((r = open_lookup(envid, rq->req_fileid, &pOpen)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}
	if ((r = file_set_size(pOpen->o_file, rq->req_size)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}
	ipc_send(envid, 0, 0, 0);
}

void serve_close(u_int envid, struct Fsreq_close *rq)
{
	struct Open *pOpen;
	int r;
	if ((r = open_lookup(envid, rq->req_fileid, &pOpen)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}
	file_close(pOpen->o_file);
	ipc_send(envid, 0, 0, 0);
}

void serve_remove(u_int envid, struct Fsreq_remove *rq)
{
	int r;
	char path[MAXPATHLEN];
	rq->req_path[MAXPATHLEN - 1] = '\0';
	strcpy(path, (char *)rq->req_path);
    r = file_remove(path);
    ipc_send(envid, r, 0, 0);
}

void serve_dirty(u_int envid, struct Fsreq_dirty *rq)
{
	struct Open *pOpen;
	int r;
	if ((r = open_lookup(envid, rq->req_fileid, &pOpen)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}
	if ((r = file_dirty(pOpen->o_file, rq->req_offset)) < 0) {
		ipc_send(envid, r, 0, 0);
		return;
	}
	ipc_send(envid, 0, 0, 0);
}

void serve_sync(u_int envid)
{
	fs_sync();
	ipc_send(envid, 0, 0, 0);
}

void serve(void)
{
    writef("fs serve stared!\n");
	u_long req = 0, whom = 0, perm = 0;
	for (;;) {
		perm = 0;
		req = ipc_recv(&whom, REQVA, &perm);
        writef("serve ipc_recv done.");
		if (!(perm & PTE_V)) {
			writef("Invalid request from %08x: no argument page\n", whom);
			continue;
		}
		switch (req) {
			case FSREQ_OPEN:
				serve_open(whom, (struct Fsreq_open *)REQVA);
				break;
			case FSREQ_MAP:
				serve_map(whom, (struct Fsreq_map *)REQVA);
				break;
			case FSREQ_SET_SIZE:
				serve_set_size(whom, (struct Fsreq_set_size *)REQVA);
				break;
			case FSREQ_CLOSE:
				serve_close(whom, (struct Fsreq_close *)REQVA);
				break;
			case FSREQ_DIRTY:
				serve_dirty(whom, (struct Fsreq_dirty *)REQVA);
				break;
			case FSREQ_REMOVE:
				serve_remove(whom, (struct Fsreq_remove *)REQVA);
				break;
			case FSREQ_SYNC:
				serve_sync(whom);
				break;
			default:
				writef("Invalid request code %d from %08x\n", whom, req);
				break;
		}
		syscall_mem_unmap(0, REQVA);
	}
}

void umain(void)
{
	user_assert(sizeof(struct File) == BY2FILE);
	writef("FS is running\n");
	writef("FS can do I/O\n");
	serve_init();
	fs_init();
	fs_test();
	serve();
}

