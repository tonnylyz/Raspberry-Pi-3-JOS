#include <pmap.h>
#include <printf.h>

void mips_init() {
    printf("init.c:\tmips_init() is called\n");

    // Lab 2 memory management initialization functions
    mips_detect_memory();
    mips_vm_init();
    page_init();
    page_check();

    panic("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");

    while (1);

    panic("init.c:\tend of mips_init() reached!");
}

void bcopy(const void *src, void *dst, size_t len) {
    void *max;
    max = dst + len;
    while (dst + 3 < max) {
        *(int *) dst = *(int *) src;
        dst += 4;
        src += 4;
    }
    while (dst < max) {
        *(char *) dst = *(char *) src;
        dst += 1;
        src += 1;
    }
}

void bzero(void *b, size_t len) {
    void *max;
    max = b + len;
    while (b + 3 < max) {
        *(int *) b = 0;
        b += 4;
    }
    while (b < max) {
        *(char *) b++ = 0;
    }
}
