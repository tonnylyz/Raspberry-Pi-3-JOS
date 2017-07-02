#include "lib.h"

int fork() {
    return syscall_fork();
}
