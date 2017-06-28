#include <printf.h>
#include <pmap.h>
#include <kclock.h>
#include <env.h>
#include <sched.h>
#include <drivers/include/uart.h>
#include <drivers/include/timer.h>
#include <drivers/include/emmc.h>
#include <syscall_all.h>

u32 get_el() {
    u32 r;
    __asm__ __volatile__ (
    "mrs %0, currentel" : "=r"(r)
    );
    return r;
}

void main() {
    uart_init();
    printf("System started!\n");

    page_init();
    printf("page_init done.\n");

    emmc_init();
    printf("emmc_init done.\n");

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
    printf("Clock tick EL : %d\n", get_el());
    sched_yield();
    setup_clock_int(0);
}

void handle_syscall(u32 no, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5) {
    sys_putchar(no, a1, a2, a3, a4, a5);
}