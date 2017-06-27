#include <printf.h>
#include <uart.h>
#include <pmap.h>

void main() {
    page_init();

    char c;
    while (1) {
        c = uart_recv();
        printcharc(c);
    }
}
