
.globl syscall_vector
syscall_vector:
    syscall_entry putchar, #0
    syscall_entry getenvid, #1