#include "lib.h"
#include <unistd.h>
#include <mmu.h>
#include <env.h>
#include <trap.h>

extern int msyscall(int, int, int, int, int, int);

void syscall_putchar(char ch) {
    msyscall(0, (int) ch, 0, 0, 0, 0);
}

u_int syscall_getenvid(void) {
    return msyscall(1, 0, 0, 0, 0, 0);
}

void syscall_yield(void) {
    msyscall(2, 0, 0, 0, 0, 0);
}

int syscall_env_destroy(u_int envid) {
    return msyscall(3, envid, 0, 0, 0, 0);
}

int syscall_set_pgfault_handler(u_int envid, void (*func)(void), u_int xstacktop) {
    return msyscall(4, envid, (int) func, xstacktop, 0, 0);
}

int syscall_mem_alloc(u_int envid, u_int va, u_int perm) {
    return msyscall(5, envid, va, perm, 0, 0);
}

int syscall_mem_map(u_int srcid, u_int srcva, u_int dstid, u_int dstva, u_int perm) {
    return msyscall(6, srcid, srcva, dstid, dstva, perm);
}

int syscall_mem_unmap(u_int envid, u_int va) {
    return msyscall(7, envid, va, 0, 0, 0);
}

int syscall_env_alloc(void) {
    int a = msyscall(8, 0, 0, 0, 0, 0);
    return a;
}

int syscall_set_env_status(u_int envid, u_int status) {
    return msyscall(9, envid, status, 0, 0, 0);
}

int syscall_set_trapframe(u_int envid, struct Trapframe *tf) {
    return msyscall(10, envid, (int) tf, 0, 0, 0);
}

void syscall_panic(char *msg) {
    msyscall(11, (int) msg, 0, 0, 0, 0);
}

int syscall_ipc_can_send(u_int envid, u_int value, u_int srcva, u_int perm) {
    return msyscall(12, envid, value, srcva, perm, 0);
}

void syscall_ipc_recv(u_int dstva) {
    msyscall(13, dstva, 0, 0, 0, 0);
}

int syscall_cgetc() {
    return msyscall(14, 0, 0, 0, 0, 0);
}
