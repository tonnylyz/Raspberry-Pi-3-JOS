.global __start

/*
 * System start at 0x00080000
 */
_start:
    mrs x7, mpidr_el1
    and x7, x7, #3
    cbz x7, _start_master
L0: wfe
    b   L0

_start_master:
    ldr x0, =0x1000000
    mov sp, x0
    bl  main
L1: b   L1
