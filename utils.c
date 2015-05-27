#include <utils.h>

// NOTE: Insecure (does not detect overlapping memory)
void memcpy(void *dest, const void *src, unsigned int n) {
    for (int i = 0; i < n; i++) {
        *(unsigned char *)dest = *(unsigned char *)src;
        ++dest;
        ++src;
    }
}

