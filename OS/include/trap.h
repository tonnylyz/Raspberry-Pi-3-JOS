#ifndef _TRAP_H_
#define _TRAP_H_

#ifndef USER_PROGRAM
#include <types.h>
#endif

struct Trapframe {
    /* Saved special registers. */
    u_long spsr;
    u_long elr;
    u_long esr;
    u_long sp;
    /* Saved main processor registers. */
    u_long regs[31];
};

void trap_init();

#endif
