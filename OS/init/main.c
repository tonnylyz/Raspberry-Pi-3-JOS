#include <printf.h>
#include <uart.h>
#include <pmap.h>

void main() {
    uart_init();
    printf("System started!\n");

    page_init();

    printf("page_init done.\n");
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
