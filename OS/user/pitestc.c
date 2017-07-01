#include <lib.h>

void umain() {
    writef("ccccccc\n");
    writef("my envid %d\n", syscall_getenvid());
    user_panic("I am done\n");
}