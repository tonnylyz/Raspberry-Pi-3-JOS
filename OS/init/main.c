#include <printf.h>
#include <uart.h>

void main() {
    uart_init();

    printf("Hello world!\n");

    char c;
    while (1) {
        c = uart_recv();
        printcharc(c);
    }
}
