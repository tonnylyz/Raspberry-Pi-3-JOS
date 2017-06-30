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

/////////////////////////////////////////////////////head
extern void umain();
extern void libmain();
extern void exit();

extern u_long* vpd;
extern u_long* vpt;

#define USED(x) (void)(x)
//////////////////////////////////////////////////////printf
#include <stdarg.h>
//#define		LP_MAX_BUF	80

void user_lp_Print(void (*output)(void *, const char *, int),
				   void *arg,
				   const char *fmt,
				   va_list ap);

void writef(char *fmt, ...);

void _user_panic(const char *, int, const char *, ...)
__attribute__((noreturn));

#define user_panic(...) _user_panic(__FILE__, __LINE__, __VA_ARGS__)


/////////////////////////////////////////////////////fork spawn
int fork(void);

void user_bcopy(const void *src, void *dst, u_long len);
void user_bzero(void *v, u_long n);
//////////////////////////////////////////////////syscall_lib
extern void syscall_putchar(char ch);
u_long syscall_getenvid(void);
void syscall_yield(void);
int syscall_env_destroy(u_long envid);
int syscall_set_pgfault_handler(u_long envid, void (*func)(void),
								u_long xstacktop);
int syscall_mem_alloc(u_long envid, u_long va, u_long perm);
int syscall_mem_map(u_long srcid, u_long srcva, u_long dstid, u_long dstva,
					u_long perm);
int syscall_mem_unmap(u_long envid, u_long va);
int syscall_env_alloc(void);
int syscall_set_env_status(u_long envid, u_long status);
int syscall_set_trapframe(u_long envid, struct Trapframe *tf);
void syscall_panic(char *msg);
int syscall_ipc_can_send(u_long envid, u_long value, u_long srcva, u_long perm);
void syscall_ipc_recv(u_long dstva);
int syscall_cgetc();

// string.c
int strlen(const char *s);
char *strcpy(char *dst, const char *src);
const char *strchr(const char *s, char c);
void *memcpy(void *destaddr, void const *srcaddr, u_long len);
int strcmp(const char *p, const char *q);

// ipc.c
void	ipc_send(u_long whom, u_long val, u_long srcva, u_long perm);
u_long	ipc_recv(u_long *whom, u_long dstva, u_long *perm);

#endif
