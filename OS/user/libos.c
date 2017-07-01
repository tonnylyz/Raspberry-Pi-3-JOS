#include "lib.h"

extern void umain();

void exit(void) {
    syscall_env_destroy(0);
}

struct Env *env;

void libmain(int argc, char **argv) {
    env = 0;
    int envid;
    envid = syscall_getenvid();
    envid = ENVX(envid);
    env = &envs[envid];
    umain(argc, argv);
    exit();
}
