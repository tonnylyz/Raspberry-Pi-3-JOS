#include <env.h>
#include <printf.h>
void sched_yield()
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
