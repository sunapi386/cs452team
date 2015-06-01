#include <utils.h>

// NOTE: Insecure (does not detect overlapping memory)
void memcpy(void *dest, const void *src, size_t n) {
    if(n == 0) { return; }
    register unsigned int i = 0;
    register unsigned int j = 0;
    for (;;)
    {
        if (i == (n>>2)) break;
        *((int *)dest + i) = *((int *)src + i);
        ++i;
    }
    while ((n-j) % 4)
    {
        *((char *)dest + i + j) = *((char *)src + i + j);
        ++j;
    }
}

// http://www.embedded.com/design/configurable-systems/4024961/Optimizing-Memcpy-improves-speed
// Listing 2: The modified-GNU algorithm
// If the source and destination pointers are both aligned on 4-byte boundaries,
// my modified-GNU algorithm copies 32 bits at a time rather than 8 bits.
static void memcpy_2(void * dst, void const * src, size_t len) {
    if(n == 0) { return; }
    long * plDst = (long *) dst;
    long const * plSrc = (long const *) src;

    if (!(src & 0xFFFFFFFC) && !(dst & 0xFFFFFFFC)) {
        while (len >= 4)  {
            *plDst++ = *plSrc++;
            len -= 4;
        }
    }

    char * pcDst = (char *) plDst;
    char const * pcDst = (char const *) plSrc;

    while (len--)  {
        *pcDst++ = *pcSrc++;
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

size_t strlen(const char *str)
{
    // OpenBSD strlen() implementation
    // http://fxr.watson.org/fxr/source/lib/libsa/strlen.c?v=OPENBSD
    const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

