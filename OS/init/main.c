#include <printf.h>
#include <uart.h>
#include <pmap.h>
#include <emmc.h>

void main() {
    uart_init();
    printf("System started!\n");

    page_init();

    printf("page_init done.\n");

    emmc_init();
    printf("emmc_init done.\n");

    char blk0[512];
    emmc_read_sector(0, blk0);

    int i;
    for (i = 0; i < 512; i++) {
        if (i % 32 == 0)
            printf("\n");
        printf("%02x ", blk0[i]);
    }

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
