#include <printf.h>
#include <pmap.h>
#include <kclock.h>
#include <env.h>
#include <sched.h>
#include <drivers/include/uart.h>
#include <drivers/include/timer.h>
#include <drivers/include/emmc.h>

u32 get_el() {
    u32 r;
    __asm__ __volatile__ (
    "mrs %0, currentel" : "=r"(r)
    );
    return r;
}

u_char program[75264];

void main() {
    uart_init();
    printf("System started!\n");

    page_init();
    printf("page_init done.\n");

    emmc_init();
    printf("emmc_init done.\n");

    env_init();
    printf("env_init done.\n");

    int i;
    u_int pos = 0;
    for (i = 0; i < 147; i++)
    {
        u_char buf[512];
        emmc_read_sector(1024 + i, buf);
        int j;
        for (j = 0; j <  512; j++) {
            program[pos + j] = buf[j];
        }
        pos += 512;
        //printf("read sec #%d\n", i);
    }

    env_create(program, 75000);

    kclock_init();
    printf("kclock_init done.\n");

    char c;
    while (1) {
        c = uart_recv();
        if (c == 0x0d) {
            printcharc('\n');
        } else if (c < 0x7f) {
            printcharc(c);
        }
    }
}

void handle_int() {
    // handle clock int only!
    clear_clock_int();
    printf("Clock tick\n");
    sched_yield();
    setup_clock_int(0);
}

void handle_pgfault() {
    printf("\nPGFAULT!\n");
    u32 r;
    __asm__ __volatile__ (
    "mrs %0, far_el1" : "=r"(r)
    );
    printf("[ERR] page fault @ [%l016x]\n", r);
    while (1) {
        empty_loop(0);
    }
}

void handle_err() {

    printf("\nKERNEL DIE!\n");
    while (1) {
        empty_loop(0);
    }

}