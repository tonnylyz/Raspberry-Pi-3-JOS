#ifndef OSLABPI_UART_H
#define OSLABPI_UART_H

#include <types.h>

extern void set_ptr(u32, u32);
extern u32  get_ptr(u32);
extern void empty_loop(u32);

#define GPFSEL1         0x3F200004
#define GPSET0          0x3F20001C
#define GPCLR0          0x3F200028
#define GPPUD           0x3F200094
#define GPPUDCLK0       0x3F200098

#define AUX_ENABLES     0x3F215004
#define AUX_MU_IO_REG   0x3F215040
#define AUX_MU_IER_REG  0x3F215044
#define AUX_MU_IIR_REG  0x3F215048
#define AUX_MU_LCR_REG  0x3F21504C
#define AUX_MU_MCR_REG  0x3F215050
#define AUX_MU_LSR_REG  0x3F215054
#define AUX_MU_MSR_REG  0x3F215058
#define AUX_MU_SCRATCH  0x3F21505C
#define AUX_MU_CNTL_REG 0x3F215060
#define AUX_MU_STAT_REG 0x3F215064
#define AUX_MU_BAUD_REG 0x3F215068

u32 uart_lsr();
u32 uart_recv();
void uart_send(u32 c);
void uart_init();
void printcharc(char);

#endif //OSLABPI_UART_H
