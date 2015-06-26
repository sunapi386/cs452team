#include <utils.h>
#include <debug.h>

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
        /*
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
    }*/
    if (n == 0) return;
    const char *_src = src;
    char *_dest = dest;

    /* Simple, byte oriented memcpy. */
    while (n--)
        *_dest++ = *_src++;
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

int CBufferPush(CBuffer *b, char ch) {
    int ret = 0;
    b->tail = (b->tail + 1) % b->size;
    if (b->tail == b->head)
    {
        // Buffer overflow warp around behavior:
        // Simply switch head and tail then proceed to insert like normal
        // since the data we have would be garbage anyways.
        // With a large buffer and regular commands it should not overflow.
        b->tail = (b->tail + 1) % b->size;
        ret = -1;
    }
    b->data[b->tail] = ch;
    return ret;
}

char CBufferPop(CBuffer *b) {
    if (b->head != b->tail)
    {
        // If no underflow, update head
        b->head = (b->head + 1) % b->size;
        return b->data[b->head];
    }
    else
    {
        //Return -1 if underflow
        return -1;
    }
}


bool CBufferIsEmpty(const CBuffer *b) {
    return b->head == b->tail;
}

void CBufferClean(CBuffer *b) {
    b->head = 0;
    b->tail = 0;
}

int CBufferPushStr(CBuffer *b, char *str)
{
    int ret = 0, counter = 0;
    while(*str)
    {
        ret = CBufferPush(b, *str);
        str++;
        counter++;
    }
    return ret == 0 ? counter : ret;
}
