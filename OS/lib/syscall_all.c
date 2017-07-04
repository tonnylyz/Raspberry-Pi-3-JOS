#include <drivers/include/uart.h>
#include <drivers/include/emmc.h>
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>

extern struct Env *curenv;

void sys_set_return(long r) {
    struct Trapframe *tf = (struct Trapframe *)(TIMESTACKTOP - sizeof(struct Trapframe));
    tf->regs[0] = r;
}

void sys_putchar(int sysno, char c) {
    printcharc(c);
}

u_long sys_getenvid() {
    return curenv->env_id;
}

void sys_yield() {
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

int sys_set_pgfault_handler(int sysno, u_int envid, u_long func, u_long xstacktop) {
    struct Env *env;
    int ret;
    ret = envid2env(envid, &env, 0);
    if (ret < 0)
        return ret;
    env->env_pgfault_handler = func;
    env->env_xstacktop = xstacktop;
    return 0;
}

int sys_mem_alloc(int sysno, u_int envid, u_long va, u_long perm) {
    struct Env *env;
    struct Page *ppage;
    int ret;
    if (va > UTOP) {
        printf("[ERR] sys_mem_alloc va\n");
        return -1;
    }
    ret = envid2env(envid, &env, 0);
    if (ret < 0) {
        printf("[ERR] sys_mem_alloc envid2env\n");
        return ret;
    }
    ret = page_alloc(&ppage);
    if (ret < 0) {
        printf("[ERR] sys_mem_alloc page_alloc\n");
        return ret;
    }
    bzero((void *)page2kva(ppage), BY2PG);
    ret = page_insert((u_long *)KADDR(env->env_pgdir), ppage, va, perm | PTE_V | ATTRIB_AP_RW_ALL);

    usleep(20000);
    if (ret < 0) {
        printf("[ERR] sys_mem_alloc page_insert\n");
        return ret;
    }
    return 0;
}

int sys_mem_map(int sysno, u_int srcid, u_long srcva, u_int dstid, u_long dstva, u_long perm) {
    int ret;
    struct Env *srcenv;
    struct Env *dstenv;
    struct Page *ppage;
    Pte *ppte;
    srcva = ROUNDDOWN(srcva, BY2PG);
    dstva = ROUNDDOWN(dstva, BY2PG);
    ret = envid2env(srcid, &srcenv, 1);
    if (ret < 0) {
        printf("[ERR] sys_mem_map envid2env\n");
        return ret;
    }
    ret = envid2env(dstid, &dstenv, 1);
    if (ret < 0) {
        printf("[ERR] sys_mem_map envid2env\n");
        return ret;
    }
    ppage = page_lookup(srcenv->env_pgdir, srcva, &ppte);
    if (ppage == NULL) {
        printf("[ERR] sys_mem_map : page_lookup : [%l016x] : [%l016x] : [%l016x]\n", srcenv->env_pgdir, srcva, &ppte);
        return -1;
    }
    ret = page_insert((u_long *)KADDR(dstenv->env_pgdir), ppage, dstva, perm);
    if (ret < 0) {
        printf("[ERR] sys_mem_map page_insert\n");
        return ret;
    }
    return 0;
}

int sys_mem_unmap(int sysno, u_int envid, u_long va) {
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

int sys_env_alloc() {
    int r;
    struct Env *e;
    r = env_alloc(&e, curenv->env_id);
    if (r < 0) {
        printf("[ERR] sys_env_alloc : env_alloc\n");
        return r;
    }
    bcopy((void *)(TIMESTACKTOP - sizeof(struct Trapframe)), &e->env_tf, sizeof(struct Trapframe));
    e->env_status = ENV_NOT_RUNNABLE;
    e->env_tf.regs[0] = 0;
    e->env_ipc_recving = 0;
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
}

void sys_panic(int sysno, char *msg) {
    panic("%s", msg);
}

void sys_ipc_recv(int sysno, u_long dstva) {
    curenv->env_ipc_dstva = dstva;
    curenv->env_ipc_recving = 1;
    curenv->env_status = ENV_NOT_RUNNABLE;
    sys_yield();
}

int sys_ipc_can_send(int sysno, u_int envid, u_long value, u_long srcva, u_long perm) {
    int r;
    struct Env *e;
    r = envid2env(envid, &e, 0);
    if (r < 0) {
        printf("[ERR] sys_ipc_can_send E_BAD_ENV\n");
        return -E_BAD_ENV;
    }
    if (!e->env_ipc_recving) {
        return -E_IPC_NOT_RECV;
    }

    if (srcva != 0 && e->env_ipc_dstva != 0) {
        Pte *pte;
        struct Page *p = page_lookup((u_long *)KADDR(curenv->env_pgdir), srcva, &pte);
        page_insert((u_long *)KADDR(e->env_pgdir), p, e->env_ipc_dstva, perm);
        e->env_ipc_perm = perm;
    }

    e->env_ipc_recving = 0;
    e->env_ipc_from = curenv->env_id;
    e->env_ipc_value = value;
    e->env_status = ENV_RUNNABLE;
    return 0;
}

int sys_cgetc() {
    return uart_recv();
}

u_long sys_pgtable_entry(int sysno, u_long va) {
    Pte *pte;
    struct Page *page;
    page = page_lookup((u_long *)KADDR(curenv->env_pgdir), va, &pte);
    if (page == NULL) {
        return 0;
    }
    return *pte;
}

u_int sys_fork() {
    struct Env *e;
    u_int envid;
    u_long va;
    u_long *pte;
    struct Page *p;
    struct Page *pp;
    envid = sys_env_alloc();
    envid2env(envid, &e, 0);
    for (va = 0; va < USTACKTOP; va += BY2PG) {
        p = page_lookup((u_long *)KADDR(curenv->env_pgdir), va, &pte);
        if (p == NULL)
            continue;
        page_alloc(&pp);
        bcopy((void *)page2kva(p), (void *)page2kva(pp), BY2PG);
        page_insert((u_long *)KADDR(e->env_pgdir), pp, va, ATTRIB_AP_RW_ALL);
    }
    e->env_status = ENV_RUNNABLE;
    return envid;
}

void sys_emmc_read(int sysno, u_long sector, u_long va) {
    struct Page *page;
    page_alloc(&page);
    u_char *buf = (u_char *)page2kva(page);
    u_int sec;
    // place fs.img at sector BASE
    u_int sector_offset = 54000 + sector;
    for (sec = 0; sec < 8; sec ++) {
        emmc_read_sector(sec + sector_offset, buf + sec * 512);
    }
    page_insert(curenv->env_pgdir, page, va, ATTRIB_AP_RW_ALL);
    usleep(20000);
    //printf("sys_emmc_read map sector #%d to [%l016x]\n", sector, va);
}