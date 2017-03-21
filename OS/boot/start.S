.global _start

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
    mov sp, #0x00100000
    bl  main
L1: b   L1
