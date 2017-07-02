.data
.global envs
envs:
	.word 0x0000008100000000 // UENVS
.globl pages
pages:
    .word 0x0000008200000000 // UPAGES
.global vpt
vpt:
	.word 0 // UVPT

.global vpd
vpd:
	.word 0 // SELF MAP!

.global __pgfault_handler
__pgfault_handler:
	.word 0 // SET LATER


.extern libmain

.text
.global _start
_start:
	bl	libmain
	nop

.text
.global __asm_pgfault_handler
__asm_pgfault_handler:
    nop