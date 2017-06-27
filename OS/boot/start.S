.globl _start

_start:
// Find CPU-0 and jump to _start_master
    mrs x0, mpidr_el1
    mov x1, #0xC1000000
    bic x0, x0, x1
    cbz x0, _start_master
    b   L0


_start_master:
// Set kernel stack
    mov sp, #0x400000
// Initialize vm
    bl  vm_init
// Enable instruction cache
    mrs x0, sctlr_el1
    orr x0, x0, #(1 << 12)
    msr sctlr_el1, x0
/*
    ldr x0, =vectors
    msr vbar_el1, x0         // init exception vector table
*/

// Memory configuration (rp2413)
    ldr x0, =0x04cc
    msr mair_el1, x0
    isb

// Load the page table base to MMU (rp2517)
    ldr x1, =0x01000000
    msr ttbr0_el1, x1
    msr ttbr1_el1, x1
    isb

// MMU configutaion (rp2480)
    mrs x2, tcr_el1
    ldr x3, =0x70040ffbf
    bic x2, x2, x3
    ldr x3, =0x2bf183f18
    orr x2, x2, x3
    msr tcr_el1, x2
    isb
// GO to main
    ldr x5, =main
    mrs x3, sctlr_el1
    ldr x4, =0x80000
    bic x3, x3, x4
    ldr x4, =0x1005
    orr x3, x3, x4
    msr sctlr_el1, x3
    isb
    br x5

L0:
    b   L0
