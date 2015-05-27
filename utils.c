#include <utils.h>

// NOTE: Insecure (does not detect overlapping memory)
void memcpy(void *dest, const void *src, unsigned int n) {
    for (int i = 0; i < n; i++) {
        *(unsigned char *)dest = *(unsigned char *)src;
        ++dest;
        ++src;
    }
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
