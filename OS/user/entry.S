.data

.p2align 12

.globl fsipcbuf
fsipcbuf:
	.space 4096
.globl fdtab
fdtab:
	.space 4096

.globl envs
envs:
	.word 0

.globl pages
pages:
	.word 0

.globl vpt
vpt:
	.word 0

.globl vpd
vpd:
	.word 0

.globl __pgfault_handler
	__pgfault_handler:
	.word 0


.extern libmain

.text
.globl _start
_start:
	bl	libmain
	nop

.text
.globl __asm_pgfault_handler
__asm_pgfault_handler:
    nop