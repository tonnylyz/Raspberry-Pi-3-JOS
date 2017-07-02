#ifndef LIB_H
#define LIB_H

typedef unsigned int u_int;
typedef unsigned long u_long;
typedef unsigned char u_char;

#define USER_PROGRAM
#include <error.h>
#include <queue.h>
#include <mmu.h>
#include <env.h>
#include <trap.h>

/////////////////////////////////////////////////////////////
//                          Table                          //
/////////////////////////////////////////////////////////////
extern u_long* vpd;

extern u_long* vpt;


/////////////////////////////////////////////////////////////
//                          Printf                         //
/////////////////////////////////////////////////////////////
#include <stdarg.h>

void user_lp_Print(void (*output)(void *, const char *, int), void *arg, const char *fmt, va_list ap);

void writef(char *fmt, ...);

void _user_panic(const char *, int, const char *, ...)
__attribute__((noreturn));

#define user_panic(...) _user_panic(__FILE__, __LINE__, __VA_ARGS__)

/////////////////////////////////////////////////////////////
//                          Fork                           //
/////////////////////////////////////////////////////////////
int fork();

/////////////////////////////////////////////////////////////
//                       Syscall                           //
/////////////////////////////////////////////////////////////
void syscall_putchar(char ch);

u_long syscall_getenvid();

void syscall_yield();

long syscall_env_destroy(u_long envid);

long syscall_set_pgfault_handler(u_long envid, void (*func)(void), u_long xstacktop);

long syscall_mem_alloc(u_long envid, u_long va, u_long perm);

long syscall_mem_map(u_long srcid, u_long srcva, u_long dstid, u_long dstva, u_long perm);

long syscall_mem_unmap(u_long envid, u_long va);

long syscall_env_alloc();

long syscall_set_env_status(u_long envid, u_long status);

long syscall_set_trapframe(u_long envid, struct Trapframe *tf);

void syscall_panic(char *msg);

long syscall_ipc_can_send(u_long envid, u_long value, u_long srcva, u_long perm);

void syscall_ipc_recv(u_long dstva);

long syscall_cgetc();

u_long syscall_pgtable_entry(u_long va);

u_int syscall_fork();

/////////////////////////////////////////////////////////////
//                         String                          //
/////////////////////////////////////////////////////////////
int strlen(const char *s);

char *strcpy(char *dst, const char *src);

const char *strchr(const char *s, char c);

void *memcpy(void *destaddr, void const *srcaddr, u_long len);

int strcmp(const char *p, const char *q);

void user_bcopy(const void *src, void *dst, u_long len);

void user_bzero(void *v, u_long n);

/////////////////////////////////////////////////////////////
//                          IPC                            //
/////////////////////////////////////////////////////////////
void ipc_send(u_long whom, u_long val, u_long srcva, u_long perm);

u_long ipc_recv(u_long *whom, u_long dstva, u_long *perm);

#endif
