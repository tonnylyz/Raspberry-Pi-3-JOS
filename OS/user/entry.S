.data
.global envs
envs:
	.word 0x90000000

.extern libmain

.text
.global _start
_start:
	bl	libmain
	nop
