#include <lib.h>

int msyscall(int no, int a1, int a2, int a3, int a4, int a5) {
    __asm__ __volatile__ (
    "svc #0"
    );
}

void syscall_putchar(char ch) {
    msyscall(0, (int) ch, 0, 0, 0, 0);
}

u_long syscall_getenvid(void) {
    return msyscall(1, 0, 0, 0, 0, 0);
}

void syscall_yield(void) {
    msyscall(2, 0, 0, 0, 0, 0);
}

int syscall_env_destroy(u_long envid) {
    return msyscall(3, envid, 0, 0, 0, 0);
}

int syscall_set_pgfault_handler(u_long envid, void (*func)(void), u_long xstacktop) {
    return msyscall(4, envid, (int) func, xstacktop, 0, 0);
}

int syscall_mem_alloc(u_long envid, u_long va, u_long perm) {
    return msyscall(5, envid, va, perm, 0, 0);
}

int syscall_mem_map(u_long srcid, u_long srcva, u_long dstid, u_long dstva, u_long perm) {
    return msyscall(6, srcid, srcva, dstid, dstva, perm);
}

int syscall_mem_unmap(u_long envid, u_long va) {
    return msyscall(7, envid, va, 0, 0, 0);
}

int syscall_env_alloc(void) {
    int a = msyscall(8, 0, 0, 0, 0, 0);
    return a;
}

int syscall_set_env_status(u_long envid, u_long status) {
    return msyscall(9, envid, status, 0, 0, 0);
}

int syscall_set_trapframe(u_long envid, struct Trapframe *tf) {
    return msyscall(10, envid, (int) tf, 0, 0, 0);
}

void syscall_panic(char *msg) {
    msyscall(11, (int) msg, 0, 0, 0, 0);
}

int syscall_ipc_can_send(u_long envid, u_long value, u_long srcva, u_long perm) {
    return msyscall(12, envid, value, srcva, perm, 0);
}

void syscall_ipc_recv(u_long dstva) {
    msyscall(13, dstva, 0, 0, 0, 0);
}

int syscall_cgetc() {
    return msyscall(14, 0, 0, 0, 0, 0);
}
