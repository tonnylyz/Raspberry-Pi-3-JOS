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
    ldr x0, =0x1000000
    mov sp, x0

// Initialize vm
    bl  vm_init

// Go to EL1

    mrs x0, currentel        // check if already in el1
    cmp x0, #4
    beq L1

    ldr x0, =0xffffff0001000000
    msr sp_el1, x0           // init the stack of el1

    // disable coprocessor traps
    mov x0, #0x33ff
    msr cptr_el2, x0         // disable coprocessor traps to el2
    msr hstr_el2, xzr        // disable coprocessor traps to el2
    mov x0, #3 << 20
    msr cpacr_el1, x0        // enable fp/simd at el1

    // initialize hcr_el2
    mov x0, #(1 << 31)
    msr hcr_el2, x0          // set el1 to 64 bit
    mov x0, #0x0800
    movk x0, #0x30d0, lsl #16
    msr sctlr_el1, x0

    // return to the el1_sp1 mode from el2
    mov x0, #0x5
    msr spsr_el2, x0         // el1_sp1 with DAIF = 0
    adr x0, L1
    msr elr_el2, x0
    eret

L1:
// Enable instruction cache
    mrs x0, sctlr_el1
    orr x0, x0, #(1 << 12)
    msr sctlr_el1, x0

// init exception vector table
    ldr x0, =vectors
    msr vbar_el1, x0

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
