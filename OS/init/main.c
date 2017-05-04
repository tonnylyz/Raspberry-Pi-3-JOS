#include <printf.h>
#include <uart.h>
#include <pmap.h>

void main() {
    uart_init();
    printf("System started!\n");

    vm_init();
    page_init();

    char c;
    while (1) {
        c = uart_recv();
        printcharc(c);
    }
}
