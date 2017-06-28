// implement fork from user space

#include "lib.h"
#include <mmu.h>
#include <env.h>

#define PAGE_FAULT_TEMP (USTACKTOP - 2 * BY2PG)

void user_bcopy(const void *src, void *dst, size_t len)
{
	void *max;
	max = dst + len;
	if (((int)src % 4 == 0) && ((int)dst % 4 == 0)) {
		while (dst + 3 < max) {
			*(int *)dst = *(int *)src;
			dst += 4;
			src += 4;
		}
	}
	while (dst < max) {
		*(char *)dst = *(char *)src;
		dst += 1;
		src += 1;
	}
}

void user_bzero(void *v, u_int n)
{
	char *p;
	int m;
	p = v;
	m = n;
	while (--m >= 0) {
		*p++ = 0;
	}
}

static void pgfault(u_int va)
{
    int r;
    va = ROUNDDOWN(va, BY2PG);

    if (!((*vpt)[VPN(va)] & PTE_COW)) {
        user_panic("pgfault : not copy on write");
    }
    //map the new page at a temporary place
    r = syscall_mem_alloc(0, PAGE_FAULT_TEMP, PTE_V | PTE_R);
    if (r < 0) {
        user_panic("pgfault : syscall_mem_alloc");
    }

    //copy the content
    user_bcopy(va, PAGE_FAULT_TEMP, BY2PG);

    //map the page on the appropriate place
    r = syscall_mem_map(0, PAGE_FAULT_TEMP, 0, va, PTE_V | PTE_R);
    if (r < 0) {
        user_panic("pgfault : syscall_mem_map");
    }

    //unmap the temporary place
    r = syscall_mem_unmap(0, PAGE_FAULT_TEMP);
    if (r < 0) {
        user_panic("pgfault : syscall_mem_unmap");
    }
}

static void duppage(u_int envid, u_int pn)
{
	int r;
    u_int addr = pn * BY2PG;
    u_int perm = ((*vpt)[pn]) & 0xfff;

    if (perm & PTE_LIBRARY) {
        writef("[LOG] duppage : PTE_LIBRARY\n");
        r = syscall_mem_map(0, addr, envid, addr, perm & PTE_LIBRARY);
        if (r < 0) {
            writef("[ERR] duppage : syscall_mem_map #-1\n");
        }
    } else if ((perm & PTE_R) != 0 || (perm & PTE_COW) != 0) {
        r = syscall_mem_map(0, addr, envid, addr, PTE_V | PTE_R | PTE_COW);
        if (r < 0) {
            writef("[ERR] duppage : syscall_mem_map #0\n");
        }
        r = syscall_mem_map(0, addr, 0,     addr, PTE_V | PTE_R | PTE_COW);
        if (r < 0) {
            writef("[ERR] duppage : syscall_mem_map #1\n");
        }
    } else {
        r = syscall_mem_map(0, addr, envid, addr, PTE_V);
        if (r < 0) {
            writef("[ERR] duppage : syscall_mem_map #3\n");
        }
    }
}

extern void __asm_pgfault_handler(void);

int fork(void)
{
	u_int envid;
	extern struct Env *envs;
	extern struct Env *env;
    int ret;

    set_pgfault_handler(pgfault);
    envid = syscall_env_alloc();
    if (envid < 0) {
        user_panic("[ERR] fork %d : syscall_env_alloc failed", ret);
        return -1;
    }

    if (envid == 0) {
        env = &envs[ENVX(syscall_getenvid())];
        return 0;
    }

    int pn;
    for (pn = 0; pn < (USTACKTOP / BY2PG) - 2; pn++) {
        if (((*vpd)[pn / PTE2PT]) != 0 && ((*vpt)[pn]) != 0) {
            duppage(envid, pn);
        }
    }

    ret = syscall_mem_alloc(envid, UXSTACKTOP - BY2PG, PTE_V | PTE_R);
    if (ret < 0) {
        user_panic("[ERR] fork %d : syscall_mem_alloc failed", envid);
        return ret;
    }

    ret = syscall_set_pgfault_handler(envid, __asm_pgfault_handler, UXSTACKTOP);
    if (ret < 0) {
        user_panic("[ERR] fork %d : syscall_set_pgfault_handler failed", envid);
        return ret;
    }

    ret = syscall_set_env_status(envid, ENV_RUNNABLE);
    if (ret < 0) {
        user_panic("[ERR] fork %d : syscall_set_env_status failed", envid);
        return ret;
    }

	return envid;
}