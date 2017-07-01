#include "lib.h"

int strlen(const char *s) {
    int n;

    for (n = 0; *s; s++) {
        n++;
    }

    return n;
}

char *strcpy(char *dst, const char *src) {
    char *ret;

    ret = dst;

    while ((*dst++ = *src++) != 0);

    return ret;
}

const char *strchr(const char *s, char c) {
    for (; *s; s++)
        if (*s == c) {
            return s;
        }

    return 0;
}

void *memcpy(void *destaddr, void const *srcaddr, u_long len) {
    char *dest = destaddr;
    char const *src = srcaddr;

    while (len-- > 0) {
        *dest++ = *src++;
    }

    return destaddr;
}


int strcmp(const char *p, const char *q) {
    while (*p && *p == *q) {
        p++, q++;
    }

    if ((u_int) *p < (u_int) *q) {
        return -1;
    }

    if ((u_int) *p > (u_int) *q) {
        return 1;
    }

    return 0;
}


void user_bcopy(const void *src, void *dst, u_long len) {
    void *max;
    max = dst + len;
    if (((int) src % 4 == 0) && ((int) dst % 4 == 0)) {
        while (dst + 3 < max) {
            *(int *) dst = *(int *) src;
            dst += 4;
            src += 4;
        }
    }
    while (dst < max) {
        *(char *) dst = *(char *) src;
        dst += 1;
        src += 1;
    }
}

void user_bzero(void *v, u_long n) {
    char *p;
    int m;
    p = v;
    m = n;
    while (--m >= 0) {
        *p++ = 0;
    }
}
