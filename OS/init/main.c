#include <printf.h>
#include <pmap.h>
#include <kclock.h>
#include <drivers/include/uart.h>
#include <drivers/include/timer.h>

void main() {
    uart_init();
    printf("System started!\n");

    page_init();
    printf("page_init done.\n");

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
    printf("Clock tick");
    setup_clock_int(0);
}