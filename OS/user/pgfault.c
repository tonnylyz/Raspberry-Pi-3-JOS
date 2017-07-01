#include "lib.h"
#include <mmu.h>

extern void (*__pgfault_handler)(u_int);

extern void __asm_pgfault_handler(void);

void set_pgfault_handler(void (*fn)(u_int va)) {
    if (__pgfault_handler == 0) {
        if (syscall_mem_alloc(0, UXSTACKTOP - BY2PG, ATTRIB_AP_RW_ALL) < 0 ||
            syscall_set_pgfault_handler(0, __asm_pgfault_handler, UXSTACKTOP) < 0) {
            writef("cannot set pgfault handler\n");
            return;
        }
    }
    __pgfault_handler = fn;
}

