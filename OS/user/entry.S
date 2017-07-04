.data
.global envs
envs:
	.dword 0x0000000090000000
.global pages
pages:
	.dword 0x00000000a0000000
.p2align 12
.global fsipcbuf
fsipcbuf:
    .space 4096


.extern libmain

.text
.global _start
_start:
	bl	libmain
	nop
