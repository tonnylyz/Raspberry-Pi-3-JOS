typedef unsigned int u32;
extern void set_ptr(u32, u32);
extern u32  get_ptr(u32);
extern void empty_loop(u32);

#define GPFSEL1         0xFFFFFF003F200004
#define GPSET0          0xFFFFFF003F20001C
#define GPCLR0          0xFFFFFF003F200028
#define GPPUD           0xFFFFFF003F200094
#define GPPUDCLK0       0xFFFFFF003F200098

#define AUX_ENABLES     0xFFFFFF003F215004
#define AUX_MU_IO_REG   0xFFFFFF003F215040
#define AUX_MU_IER_REG  0xFFFFFF003F215044
#define AUX_MU_IIR_REG  0xFFFFFF003F215048
#define AUX_MU_LCR_REG  0xFFFFFF003F21504C
#define AUX_MU_MCR_REG  0xFFFFFF003F215050
#define AUX_MU_LSR_REG  0xFFFFFF003F215054
#define AUX_MU_MSR_REG  0xFFFFFF003F215058
#define AUX_MU_SCRATCH  0xFFFFFF003F21505C
#define AUX_MU_CNTL_REG 0xFFFFFF003F215060
#define AUX_MU_STAT_REG 0xFFFFFF003F215064
#define AUX_MU_BAUD_REG 0xFFFFFF003F215068

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

int main() {
    u32 ra;
    uart_init();
    int i;
    
    for (i = 0; i < 10; i++)
        uart_send((u32)('.'));
    uart_send(0x0D);
    uart_send(0x0A);
    for (i = 0; i < 10; i++)
        uart_send((u32)('.'));
    uart_send(0x0D);
    uart_send(0x0A);
    for (i = 0; i < 10; i++)
        uart_send((u32)('.'));
    uart_send(0x0D);
    uart_send(0x0A);
    for (i = 0; i < 10; i++)
        uart_send((u32)('.'));
    uart_send(0x0D);
    uart_send(0x0A);
    while (1) {
        ra = uart_recv();
        if (ra == 0x0D) uart_send(0x0A);
        uart_send(ra);
    }
}
