#include <mmu.h>
#include <error.h>
#include <env.h>
#include <kerelf.h>
#include <sched.h>
#include <pmap.h>
#include <printf.h>

struct Env *envs = NULL;		// All environments
struct Env *curenv = NULL;	        // the current env

static struct Env_list env_free_list;	// Free list


u_int mkenvid(struct Env *e)
{
	static u_long next_env_id = 0;
	u_int idx = e - envs;
	return (++next_env_id << (1 + LOG2NENV)) | idx;
}

int envid2env(u_int envid, struct Env **penv, int checkperm)
{
	struct Env *e;
	if (envid == 0) {
		*penv = curenv;
		return 0;
	}
	e = &envs[ENVX(envid)];
	if (e->env_status == ENV_FREE || e->env_id != envid) {
		*penv = 0;
		return -E_BAD_ENV;
	}
    if (checkperm && e != curenv && e->env_parent_id != curenv->env_id){
        *penv = 0;
        return -E_BAD_ENV;
    }
	*penv = e;
	return 0;
}

void env_init(void)
{
    LIST_INIT(&env_free_list);
    int i;
    for (i = NENV - 1; i >= 0; i--) {
        envs[i].env_id = i;
        envs[i].env_status = ENV_FREE;
        LIST_INSERT_HEAD(&env_free_list, &envs[i], env_link);
    }
}

static int env_setup_vm(struct Env *e)
{
	int i, r;
	struct Page *p = NULL;
	u_long *pgdir;
	if ((r = page_alloc(&p)) < 0) {
		panic("env_setup_vm - page_alloc error\n");
		return r;
	}
	p->pp_ref++;
	pgdir = (Pde *)page2pa(p);
    // TODO: set ttbr
	return 0;
}

int env_alloc(struct Env **new, u_int parent_id)
{
	int r;
	struct Env *e;
    e = LIST_FIRST(&env_free_list);
    if (e == NULL)
        return -E_NO_FREE_ENV;
    r = env_setup_vm(e);
    if (r < 0)
        return r;
    e->env_id = mkenvid(e);
    e->env_parent_id = parent_id;
    e->env_status = ENV_RUNNABLE;

    // TODO: set spsr and sp

    LIST_REMOVE(e, env_link);
    *new = e;
    return 0;
}

static int load_icode_mapper(u_long va, u_int32_t sgsize, u_char *bin, u_int32_t bin_size, void *user_data)
{
	struct Env *env = (struct Env *)user_data;
	struct Page *p = NULL;
	u_long i;
	int r;
	u_long offset = va - ROUNDDOWN(va, BY2PG);
	for (i = 0; i < bin_size; i += BY2PG) {
        r = page_alloc(&p);
        if (r < 0) {
            panic("Allocate page failed.");
            return r;
        }
        p->pp_ref++;
        r = page_insert(env->env_pgdir, p, va - offset + i, PTE_V | PTE_R);
        if (r < 0) {
            panic("Insert page failed.");
            return r;
        }
        bcopy(bin + i, (void *) page2kva(p) + offset, MIN(BY2PG, bin_size - i));
	}
	while (i < sgsize) {
        r = page_alloc(&p);
        if (r < 0) {
            panic("Allocate page failed.");
            return r;
        }
        p->pp_ref++;
        r = page_insert(env->env_pgdir, p, va - offset + i, PTE_V | PTE_R);
        if (r < 0) {
            panic("Insert page failed.");
            return r;
        }
        i += BY2PG;
    }
	return 0;
}

static void load_icode(struct Env *e, u_char *binary, u_int size)
{
	struct Page *p = NULL;
	u_long entry_point;
	u_long r;
    u_long perm;
    r = page_alloc(&p);
    if (r < 0) {
        panic("Allocate page failed.");
        return r;
    }
    r = page_insert(e->env_pgdir, p, USTACKTOP - BY2PG, PTE_V | PTE_R);
    if (r < 0) {
        panic("Insert page failed.");
        return r;
    }
    r = load_elf(binary, size, &entry_point, e, load_icode_mapper);
    if (r < 0) {
        panic("Load elf failed.");
        return r;
    }
    // set elr instead of pc
	e->env_tf.pc = entry_point;
}

void env_create(u_char *binary, int size)
{
	struct Env *e;
    env_alloc(&e, 0);
    load_icode(e, binary, size);
}

void env_free(struct Env *e)
{
	// TODO: iterate page table and page_remove
	e->env_status = ENV_FREE;
	LIST_INSERT_HEAD(&env_free_list, e, env_link);
}

void env_destroy(struct Env *e)
{
	env_free(e);
	if (curenv == e) {
		curenv = NULL;
		/*bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),
			  (void *)TIMESTACK - sizeof(struct Trapframe),
			  sizeof(struct Trapframe));*/
		sched_yield();
	}
}

void env_run(struct Env *e)
{
    struct Trapframe *old = (struct Trapframe *) (TIMESTACKTOP - sizeof(struct Trapframe));
    if (curenv) {
        bcopy(old, &(curenv->env_tf), sizeof(struct Trapframe));
        curenv->env_tf.pc = old->cp0_epc;
    }
    curenv = e;
    bcopy(&curenv->env_tf, old, sizeof(struct Trapframe));
    // TODO: switching context
}