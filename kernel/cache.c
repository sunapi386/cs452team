#include <kernel/cache.h>

void cacheEnable() {
    asm volatile(
        "mrc p15, 0, r0, c1, c0, 0\n\t"
        "ldr r1, =0x40001004\n\t"
        "orr r0, r0, r1\n\t"
        "mcr p15, 0, r0, c1, c0, 0\n\t"
    );
}

void cacheDisable() {
    asm volatile(
        "mrc p15, 0, r0, c1, c0, 0\n\t"
        "ldr r1, =0xbfffeffb\n\t"
        "and r0, r0, r1\n\t"
        "mcr p15, 0, r0, c1, c0, 0\n\t"
    );
}
