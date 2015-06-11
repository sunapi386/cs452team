#include <utils.h>

// NOTE: Insecure (does not detect overlapping memory)
void memcpy(void *dest, const void *src, size_t n) {
    /*
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
    );*/
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

int countLeadingZeroes(const unsigned int mask) {
    static const int table[32] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return table[(unsigned int)((mask ^ (mask & (mask - 1))) * 0x077cb531u) >> 27];
}

void CBufferInit(CBuffer *b, char *array, size_t size) {
    b->data = array;
    b->size = size;
    CBufferClean(b);
}

void CBufferPush(CBuffer *b, char ch) {
    if ((b->tail + 1) % b->size == b->head)
    {
        // Buffer overflow warp around behavior:
        // Simply switch head and tail then proceed to insert like normal
        // since the data we have would be garbage anyways.
        // With a large buffer and regular commands it should not overflow.
        b->tail = b->head;
    }
    b->tail = (b->tail + 1) % b->size;
    b->data[b->tail] = ch;
}

char CBufferPop(CBuffer *b) {
    if (b->head == b->tail)
    {
        return '\0';
    }
    else if ((b->head + 1) % b->size != (b->tail + 1) % b->size)
    {
        // If no underflow, update head
        b->head = (b->head + 1) % b->size;
        return b->data[b->head];
    }
    else
    {
        //Return NUL if underflow
        return'\0';
    }
}


bool CBufferIsEmpty(CBuffer *b) {
    return ((b->head + 1) % b->size == (b->tail + 1) % b->size) ||
           ((b->head == b->tail) && b->head == 0);
}

void CBufferClean(CBuffer *b) {
    b->head = 0;
    b->tail = 0;
}
