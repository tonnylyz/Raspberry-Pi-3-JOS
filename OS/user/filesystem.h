#ifndef OSLABPI_FILESYSTEM_H
#define OSLABPI_FILESYSTEM_H

#define BY2BLK        BY2PG
#define BIT2BLK        (BY2BLK*8)
#define BY2SECT        512
#define SECT2BLK    (BY2BLK/BY2SECT)

#define DISKMAP        0xb0000000
#define DISKMAX        0xf0000000


#define MAXNAMELEN    128

#define MAXPATHLEN    1024

#define NDIRECT        10
#define NINDIRECT    (BY2BLK/4)

#define MAXFILESIZE    (NINDIRECT*BY2BLK)

#define BY2FILE     256

struct File {
    u_char f_name[MAXNAMELEN];
    u_int f_size;
    u_int f_type;
    u_int f_direct[NDIRECT];
    u_int f_indirect;

    struct File *f_dir;
    u_char f_pad[BY2FILE - MAXNAMELEN - 4 - 4 - NDIRECT * 4 - 8 - 8];
};

#define FILE2BLK    (BY2BLK/sizeof(struct File))

#define FTYPE_REG        0
#define FTYPE_DIR        1

// BIG ENDIAN 97602868
#define FS_MAGIC    0x68286097

struct Super {
    u_int s_magic;
    u_int s_nblocks;
    struct File s_root;
};

#define FSREQ_OPEN     1
#define FSREQ_MAP      2
#define FSREQ_SET_SIZE 3
#define FSREQ_CLOSE    4
#define FSREQ_DIRTY    5
#define FSREQ_REMOVE   6
#define FSREQ_SYNC     7

struct Fsreq_open {
    char req_path[MAXPATHLEN];
    u_int req_omode;
};

struct Fsreq_map {
    int req_fileid;
    u_int req_offset;
};

struct Fsreq_set_size {
    int req_fileid;
    u_int req_size;
};

struct Fsreq_close {
    int req_fileid;
};

struct Fsreq_dirty {
    int req_fileid;
    u_int req_offset;
};

struct Fsreq_remove {
    u_char req_path[MAXPATHLEN];
};

#endif //OSLABPI_FILESYSTEM_H
