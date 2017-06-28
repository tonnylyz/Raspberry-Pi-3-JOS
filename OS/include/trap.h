#ifndef _TRAP_H_
#define _TRAP_H_

#include <types.h>

struct Trapframe {
    /* Saved special registers. */
    u_long spsr;
    u_long elr;
    u_long esr;
    u_long sp;
    /* Saved main processor registers. */
    u_long regs[31];
};
void *set_except_vector(int n, void *addr);
void trap_init();

#endif
