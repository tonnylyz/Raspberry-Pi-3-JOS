.global get_ptr
get_ptr:
    ldr x0, [x0]
    ret

.global set_ptr
set_ptr:
    str w1, [x0]
    ret

.global empty_loop
empty_loop:
    ret