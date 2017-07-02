#include <lib.h>

extern u_long msyscall(u_long no, u_long a1, u_long a2, u_long a3, u_long a4, u_long a5);

void syscall_putchar(char ch) {
    msyscall(0, (u_long)ch, 0, 0, 0, 0);
}

u_long syscall_getenvid() {
    return msyscall(1, 0, 0, 0, 0, 0);
}

void syscall_yield() {
    msyscall(2, 0, 0, 0, 0, 0);
}

long syscall_env_destroy(u_long envid) {
    return (long)msyscall(3, envid, 0, 0, 0, 0);
}

long syscall_set_pgfault_handler(u_long envid, void (*func)(void), u_long xstacktop) {
    return (long)msyscall(4, envid, (u_long)func, xstacktop, 0, 0);
}

long syscall_mem_alloc(u_long envid, u_long va, u_long perm) {
    return (long)msyscall(5, envid, va, perm, 0, 0);
}

long syscall_mem_map(u_long srcid, u_long srcva, u_long dstid, u_long dstva, u_long perm) {
    return (long)msyscall(6, srcid, srcva, dstid, dstva, perm);
}

long syscall_mem_unmap(u_long envid, u_long va) {
    return (long)msyscall(7, envid, va, 0, 0, 0);
}

long syscall_env_alloc() {
    return (long)msyscall(8, 0, 0, 0, 0, 0);
}

long syscall_set_env_status(u_long envid, u_long status) {
    return (long)msyscall(9, envid, status, 0, 0, 0);
}

long syscall_set_trapframe(u_long envid, struct Trapframe *tf) {
    return (long)msyscall(10, envid, (u_long)tf, 0, 0, 0);
}

void syscall_panic(char *msg) {
    msyscall(11, (u_long)msg, 0, 0, 0, 0);
}

long syscall_ipc_can_send(u_long envid, u_long value, u_long srcva, u_long perm) {
    return (long)msyscall(12, envid, value, srcva, perm, 0);
}

void syscall_ipc_recv(u_long dstva) {
    msyscall(13, dstva, 0, 0, 0, 0);
}

long syscall_cgetc() {
    return (long)msyscall(14, 0, 0, 0, 0, 0);
}

u_long syscall_pgtable_entry(u_long va) {
    return (long)msyscall(15, va, 0, 0, 0, 0);
}