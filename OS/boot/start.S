.globl _start
_start:
    b   _start_mask
.space 0x7ffc,0
_start_mask:
    mrs x0, mpidr_el1
    mov x1, #0xC1000000
    bic x0, x0, x1
    cbz x0, _start_master
    b   L0

_start_master:
    mov sp, #0x08000000
    bl  main

L0:
    b   L0
