#include <trap.h>
#include <env.h>
#include <printf.h>

//extern void handle_int();
extern void handle_reserved();
extern void handle_tlb();
extern void handle_sys();
extern void handle_mod();
unsigned long exception_handlers[32];

void trap_init()
{

}

void *set_except_vector(int n, void *addr)
{

}

struct pgfault_trap_frame {
    u_int fault_va;
    u_int err;
    u_int sp;
    u_int eflags;
    u_int pc;
    u_int empty1;
    u_int empty2;
    u_int empty3;
    u_int empty4;
    u_int empty5;
};


void page_fault_handler(struct Trapframe *tf)
{

}
