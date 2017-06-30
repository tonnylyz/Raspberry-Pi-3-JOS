#include "lib.h"

extern struct Env *env;

void ipc_send(u_long whom, u_long val, u_long srcva, u_long perm)
{
	int r;
	while ((r = syscall_ipc_can_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV) {
		syscall_yield();
	}
	if (r == 0) {
		return;
	}
	user_panic("error in ipc_send: %d", r);
}

u_long ipc_recv(u_long *whom, u_long dstva, u_long *perm)
{
	syscall_ipc_recv(dstva);
	if (whom) {
		*whom = env->env_ipc_from;
	}
	if (perm) {
		*perm = env->env_ipc_perm;
	}
	return env->env_ipc_value;
}

