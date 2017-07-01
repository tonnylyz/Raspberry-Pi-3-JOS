#include "lib.h"

#define PAGE_FAULT_TEMP (USTACKTOP - 2 * BY2PG)


static void pgfault(u_int va) {/*
    int r;
    va = ROUNDDOWN(va, BY2PG);

    if (!((*vpt)[VPN(va)] & ATTRIB_AP_RW_ALL)) {
        user_panic("pgfault : not copy on write");
    }
    //map the new page at a temporary place
    r = syscall_mem_alloc(0, PAGE_FAULT_TEMP, ATTRIB_AP_RW_ALL);
    if (r < 0) {
        user_panic("pgfault : syscall_mem_alloc");
    }

    //copy the content
    user_bcopy(va, PAGE_FAULT_TEMP, BY2PG);

    //map the page on the appropriate place
    r = syscall_mem_map(0, PAGE_FAULT_TEMP, 0, va, ATTRIB_AP_RW_ALL);
    if (r < 0) {
        user_panic("pgfault : syscall_mem_map");
    }

    //unmap the temporary place
    r = syscall_mem_unmap(0, PAGE_FAULT_TEMP);
    if (r < 0) {
        user_panic("pgfault : syscall_mem_unmap");
    }*/
}

static void duppage(u_int envid, u_int pn) {/*
	int r;
    u_int addr = pn * BY2PG;
    u_int perm = ((*vpt)[pn]) & 0xfff;

    if (perm & ATTRIB_AP_RW_ALL) {
        writef("[LOG] duppage : PTE_LIBRARY\n");
        r = syscall_mem_map(0, addr, envid, addr, perm & ATTRIB_AP_RW_ALL);
        if (r < 0) {
            writef("[ERR] duppage : syscall_mem_map #-1\n");
        }
    } else if ((perm & PTE_R) != 0 || (perm & ATTRIB_AP_RW_ALL) != 0) {
        r = syscall_mem_map(0, addr, envid, addr, PTE_V | ATTRIB_AP_RW_ALL);
        if (r < 0) {
            writef("[ERR] duppage : syscall_mem_map #0\n");
        }
        r = syscall_mem_map(0, addr, 0,     addr, PTE_V | ATTRIB_AP_RW_ALL);
        if (r < 0) {
            writef("[ERR] duppage : syscall_mem_map #1\n");
        }
    } else {
        r = syscall_mem_map(0, addr, envid, addr, PTE_V);
        if (r < 0) {
            writef("[ERR] duppage : syscall_mem_map #3\n");
        }
    }
    */
}

extern void __asm_pgfault_handler(void);

int fork(void) {
    u_int envid;
    extern struct Env *envs;
    extern struct Env *env;
    int ret;

    set_pgfault_handler(pgfault);
    envid = syscall_env_alloc();
    if (envid < 0) {
        user_panic("[ERR] fork %d : syscall_env_alloc failed", ret);
    }

    if (envid == 0) {
        env = &envs[ENVX(syscall_getenvid())];
        return 0;
    }

    /*int pn;
    for (pn = 0; pn < (USTACKTOP / BY2PG) - 2; pn++) {
        if (((*vpd)[pn / PTE2PT]) != 0 && ((*vpt)[pn]) != 0) {
            duppage(envid, pn);
        }
    }

    ret = syscall_mem_alloc(envid, UXSTACKTOP - BY2PG, ATTRIB_AP_RW_ALL);
    if (ret < 0) {
        user_panic("[ERR] fork %d : syscall_mem_alloc failed", envid);
    }

    ret = syscall_set_pgfault_handler(envid, __asm_pgfault_handler, UXSTACKTOP);
    if (ret < 0) {
        user_panic("[ERR] fork %d : syscall_set_pgfault_handler failed", envid);
    }
    */
    ret = syscall_set_env_status(envid, ENV_RUNNABLE);
    if (ret < 0) {
        user_panic("[ERR] fork %d : syscall_set_env_status failed", envid);
    }

    return envid;
}
