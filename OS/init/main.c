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

u32 get_esr() {
    u32 r;
    __asm__ __volatile__ (
    "mrs %0, esr_el1" : "=r"(r)
    );
    return r;
}

u64 get_far() {
    u64 r;
    __asm__ __volatile__ (
    "mrs %0, far_el1" : "=r"(r)
    );
    return r;
}

u_char program[512 * 300];
void load_program(u_int sector) {
    int i;
    u_int pos = 0;
    for (i = 0; i < 300; i++)
    {
        u_char buf[512];
        emmc_read_sector(sector + i, buf);
        int j;
        for (j = 0; j <  512; j++) {
            program[pos + j] = buf[j];
        }
        pos += 512;
    }
}


void main() {
    uart_init();
    printf("System started!\n");

    page_init();
    printf("page_init done.\n");

    emmc_init();
    printf("emmc_init done.\n");

    env_init();
    printf("env_init done.\n");

    // trap_init: trap was initialized implicitly in `utility.S`

    // Load elf image from sd card first
    // dd if=[elf image] of=/dev/sd[x] seek=1024 bs=512
    // sec  name    size
    // 51000 fstest  101388
    // 52000 serv    130542

    // fstest
    load_program(51000);
    env_create(program, 101388);

    // serv
    load_program(52000);
    env_create(program, 130542);

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
    sched_yield();
    setup_clock_int(0);
}

extern void user_pgfault_handler(u64 function, u64 xstacktop, u64 ret);

void handle_pgfault() {
    printf("\n[System Exception]\n");
    printf("Page fault : va : [%l016x]\n", get_far());
    printf("esr : [%08x]\n", get_esr());
    while (1) {
        empty_loop(0);
    }
}

void handle_err() {
    printf("\n[System Exception]\n");
    printf("Kernel died\n");
    printf("esr : [%08x]\n", get_esr());
    printf("far : [%l016x]\n", get_far());
    while (1) {
        empty_loop(0);
    }

}