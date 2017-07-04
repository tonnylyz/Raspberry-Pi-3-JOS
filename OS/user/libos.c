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
    envs = (struct Env *)((u_long)UENVS & 0xFFFFFFFF);
    env = &envs[envid];
    env = (struct Env *)((u_long)env & 0xFFFFFFFF);
    umain(argc, argv);
    exit();
}
