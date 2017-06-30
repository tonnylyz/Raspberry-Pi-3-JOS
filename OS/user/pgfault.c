// User-level page fault handler.  We use an assembly wrapper around a C function.
// The assembly wrapper is in entry.S.

#include "lib.h"
#include <mmu.h>

extern void (*__pgfault_handler)(u_int);
extern void __asm_pgfault_handler(void);


//
// Set the page fault handler function.
// If there isn't one yet, _pgfault_handler will be 0.
// The first time we register a handler, we need to
// allocate an exception stack and tell the kernel to
// call _asm_pgfault_handler on it.
//
void
set_pgfault_handler(void (*fn)(u_int va))
{
	if (__pgfault_handler == 0) {
		if (syscall_mem_alloc(0, UXSTACKTOP - BY2PG, ATTRIB_AP_RW_ALL) < 0 ||
			syscall_set_pgfault_handler(0, __asm_pgfault_handler, UXSTACKTOP) < 0) {
			writef("cannot set pgfault handler\n");
			return;
		}
	}
	__pgfault_handler = fn;
}

