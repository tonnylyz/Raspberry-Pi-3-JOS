#include <env.h>
#include <pmap.h>

void sched_yield()
{
    static int i = 0;
    while (1) {
        i++;
        i = i % NENV;
        if (envs[i].env_status == ENV_RUNNABLE) {
            printf("RUN @ [%l016x]\n", envs[i].env_tf.elr);
            env_run(&envs[i]);
            return;
        }
    }
}
