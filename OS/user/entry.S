.data
.global envs
envs:
	.word 0xFFFFFF0001700000

.extern libmain

.text
.global _start
_start:
	bl	libmain
	nop
