#include <utils.h>

// NOTE: Insecure (does not detect overlapping memory)
void memcpy(void *dest, const void *src, size_t n) {
    if(n == 4) {
        *((unsigned int *)dest) = *((unsigned int *)src);
        return;
    }

    switch(n % 4) {
        case 3:
            n--;
            *(((char *)dest + n)) = *(((char *)src + n));
        case 2:
            n--;
            *(((char *)dest + n)) = *(((char *)src + n));
        case 1:
            n--;
            *(((char *)dest + n)) = *(((char *)src + n));
    }

    if(n == 0) { return; }

    register void  *r_d asm("r0") = dest;
    register const void  *r_s asm("r1") = src;
    register size_t r_n asm("r2") = n;

    asm volatile(
        "memcpy_:\n\t"
        "ldmia %1!, {r3-r6}\n\t"
        "stmia %0!, {r3-r6}\n\t"
        "subs %2, %2, #16\n\t"
        "bgt memcpy_\n\t"
        : // no output
        : "r"(r_d), "r"(r_s), "r"(r_n) // input regs
        : "r3", "r4", "r5", "r6" // scratch
    );
}

int strcmp (const char * dst, const char * src) {
    // http://en.wikibooks.org/wiki/C_Programming/C_Reference/string.h/strcmp
    for(; *dst == *src; ++dst, ++src)
        if(*dst == 0)
            return 0;
    return *(unsigned char *)dst < *(unsigned char *)src ? -1 : 1;
}

char * strncpy(char *dst, const char *src, size_t n) {
    // http://opensource.apple.com/source/Libc/Libc-262/ppc/gen/strncpy.c
    char *s = dst;
    while (n > 0 && *src != '\0') {
        *s++ = *src++;
        --n;
    }
    while (n > 0) {
        *s++ = '\0';
        --n;
    }
    return dst;
}

size_t strlen(const char *str)
{
    // OpenBSD strlen() implementation
    // http://fxr.watson.org/fxr/source/lib/libsa/strlen.c?v=OPENBSD
    const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

