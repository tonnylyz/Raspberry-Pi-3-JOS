#include <env.h>
#include <pmap.h>

void sched_yield(void)
{
    static int i = 0;
    while (1) {
        i++;
        i = i % NENV;
        if (envs[i].env_status == ENV_RUNNABLE) {
            env_run(&envs[i]);
            return;
        }
    }
}