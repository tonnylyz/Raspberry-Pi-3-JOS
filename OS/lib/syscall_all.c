#include <syscall_all.h>
#include <drivers/include/uart.h>
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>

extern char *KERNEL_SP;
extern struct Env *curenv;

static void *memcpy(void *destaddr, void const *srcaddr, u_int len) {
    char *dest = destaddr;
    char const *src = srcaddr;

    while (len-- > 0) {
        *dest++ = *src++;
    }

    return destaddr;
}

void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5) {
    printcharc((char) c);
    return;
}

u_int sys_getenvid(void) {
    return curenv->env_id;
}

void sys_yield(void) {
    bcopy((u_int) KERNEL_SP - sizeof(struct Trapframe),
          TIMESTACK - sizeof(struct Trapframe),
          sizeof(struct Trapframe));

    sched_yield();
}

int sys_env_destroy(int sysno, u_int envid) {
    int r;
    struct Env *e;

    if ((r = envid2env(envid, &e, 1)) < 0) {
        return r;
    }

    env_destroy(e);
    return 0;
}

int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop) {
    struct Env *env;
    int ret;
    ret = envid2env(envid, &env, 0);
    if (ret < 0)
        return ret;
    env->env_pgfault_handler = func;

    xstacktop = TRUP(xstacktop);
    env->env_xstacktop = xstacktop;

    return 0;
}

int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm) {
    struct Env *env;
    struct Page *ppage;
    int ret;
    if (perm & PTE_V == 0 || perm & PTE_COW != 0) {
        printf("[ERR] perm\n");
        return -E_INVAL;
    }
    if (va >= UTOP) {
        printf("[ERR] va\n");
        return -1;
    }
    // Get env
    ret = envid2env(envid, &env, 0);
    if (ret < 0) {
        printf("[ERR] envid2env\n");
        return ret;
    }
    // Allocate page
    ret = page_alloc(&ppage);
    if (ret < 0) {
        printf("[ERR] page_alloc\n");
        return ret;
    }
    // Clean page
    bzero(page2kva(ppage), BY2PG);
    // Insert page to env
    ret = page_insert(env->env_pgdir, ppage, va, perm | PTE_R | PTE_V);
    if (ret < 0) {
        printf("[ERR] page_insert\n");
        return ret;
    }
    return 0;
}

int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva, u_int perm) {
    int ret;
    u_int round_srcva, round_dstva;
    struct Env *srcenv;
    struct Env *dstenv;
    struct Page *ppage;
    Pte *ppte;

    ppage = NULL;
    ret = 0;
    round_srcva = ROUNDDOWN(srcva, BY2PG);
    round_dstva = ROUNDDOWN(dstva, BY2PG);
    // va restriction
    if (srcva >= UTOP || dstva >= UTOP || srcva != round_srcva || dstva != round_dstva) {
        printf("[ERR] va\n");
        return -1;
    }
    // Get env
    ret = envid2env(srcid, &srcenv, 1);
    if (ret < 0) {
        printf("[ERR] envid2env\n");
        return ret;
    }
    ret = envid2env(dstid, &dstenv, 1);
    if (ret < 0) {
        printf("[ERR] envid2env\n");
        return ret;
    }
    // Lookup the page in the source env
    ppage = page_lookup(srcenv->env_pgdir, srcva, &ppte);
    if (ppage == NULL) {
        printf("[ERR] sys_mem_map : page_lookup : [%8x] : [%8x] : [%8x]\n", srcenv->env_pgdir, srcva, &ppte);
        return -1;
    }
    // Insert the page to the destination env
    ret = page_insert(dstenv->env_pgdir, ppage, dstva, perm);
    if (ret < 0) {
        printf("[ERR] page_insert\n");
        return ret;
    }
    return 0;
}

int sys_mem_unmap(int sysno, u_int envid, u_int va) {
    int ret;
    struct Env *env;
    ret = envid2env(envid, &env, 1);
    if (ret < 0) {
        printf("[ERR] sys_mem_unmap : envid2env\n");
        return ret;
    }
    page_remove(env->env_pgdir, va);
    return ret;
}

int sys_env_alloc(void) {
    int r;
    struct Env *e;
    r = env_alloc(&e, curenv->env_id);
    if (r < 0) {
        return r;
    }
    bcopy(KERNEL_SP - sizeof(struct Trapframe), &e->env_tf, sizeof(struct Trapframe));
    Pte *ppte = NULL;
    pgdir_walk(curenv->env_pgdir, USTACKTOP - BY2PG, 0, &ppte);
    if (ppte != NULL) {
        struct Page *ppc, *ppp;
        ppp = pa2page(PTE_ADDR(*ppte));
        page_alloc(&ppc);
        bcopy(page2kva(ppp), page2kva(ppc), BY2PG);
        page_insert(e->env_pgdir, ppc, USTACKTOP - BY2PG, PTE_R | PTE_V);
    }
    e->env_tf.pc = e->env_tf.cp0_epc;
    e->env_status = ENV_NOT_RUNNABLE;
    e->env_tf.regs[2] = 0;
    return e->env_id;
}

int sys_set_env_status(int sysno, u_int envid, u_int status) {
    struct Env *env;
    int ret;
    if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE && status != ENV_FREE) {
        return -E_INVAL;
    }
    ret = envid2env(envid, &env, 0);
    if (ret < 0)
        return ret;
    env->env_status = status;
    return 0;
}

int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf) {
    panic("sys_set_trapframe not implemented!");
    return 0;
}

void sys_panic(int sysno, char *msg) {
    panic("%s", msg);
}

void sys_ipc_recv(int sysno, u_int dstva) {
    if (dstva > UTOP) {
        return;
    }
    curenv->env_ipc_dstva = dstva;
    curenv->env_ipc_recving = 1;
    curenv->env_status = ENV_NOT_RUNNABLE;
    sys_yield();
}

int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva, u_int perm) {
    int r;
    struct Env *e;
    struct Page *p;
    r = envid2env(envid, &e, 0);
    if (r < 0) {
        return -E_BAD_ENV;
    }
    if (!e->env_ipc_recving) {
        return -E_IPC_NOT_RECV;
    }
    e->env_ipc_recving = 0;
    e->env_ipc_from = curenv->env_id;
    e->env_ipc_value = value;
    e->env_status = ENV_RUNNABLE;
    return 0;
}

