#ifndef _TRAP_H_
#define _TRAP_H_

#ifndef USER_PROGRAM
#include <types.h>
#endif

struct Trapframe
{
    u_long spsr;
    u_long elr;
    u_long esr;
    u_long sp;
    u_long regs[31];
};

#endif
