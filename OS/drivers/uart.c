#include <uart.h>

u32 uart_lsr() {
    return get_ptr(AUX_MU_LSR_REG);
}

u32 uart_recv() {
    while (1) {
        if (uart_lsr() & 0x01) break;
    }
    return (get_ptr(AUX_MU_IO_REG) & 0xFF);
}

void uart_send(u32 c) {
    while (1) {
        if (uart_lsr() & 0x20) break;
    }
    set_ptr(AUX_MU_IO_REG, c);
}

void uart_init() {
    u32 ra;
    set_ptr(AUX_ENABLES, 1);
    set_ptr(AUX_MU_IER_REG, 0);
    set_ptr(AUX_MU_CNTL_REG, 0);
    set_ptr(AUX_MU_LCR_REG, 3);
    set_ptr(AUX_MU_MCR_REG, 0);
    set_ptr(AUX_MU_IER_REG, 0);
    set_ptr(AUX_MU_IIR_REG, 0xC6);
    set_ptr(AUX_MU_BAUD_REG, 270);
    ra = get_ptr(GPFSEL1);
    ra &= ~(7 << 12); //gpio14
    ra |= 2 << 12;    //alt5
    ra &= ~(7 << 15); //gpio15make
    ra |= 2 << 15;    //alt5
    set_ptr(GPFSEL1, ra);
    set_ptr(GPPUD, 0);
    for (ra = 0; ra < 150; ra++) empty_loop(ra);
    set_ptr(GPPUDCLK0, (1 << 14) | (1 << 15));
    for (ra = 0; ra < 150; ra++) empty_loop(ra);
    set_ptr(GPPUDCLK0, 0);
    set_ptr(AUX_MU_CNTL_REG, 3);
}

void printcharc(char c) {
    if (c == 0x0D) {
        uart_send(0x0A);
    }
    uart_send(c);
}