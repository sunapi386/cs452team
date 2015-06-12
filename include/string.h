#ifndef __STRING_H
#define __STRING_H
#include <debug.h>
/**
How ahould we handle the train controller

Bandwidth of communication with the train controller will probably be the
limiting factor in your trains project.

Use it wisely, which means as little as possible.
Any time you can substitute computation for communication you should do so.
Communication with the train controller occurs in muli-byte messages.
It makes no sense to pass around half a message.
Reduce the number of system calls.
Minimizes bugs that occur when putting parts of messages together.
Convert input bytes into messages as early as possible.
Unpack output messages into bytes as late as possible.
*/

#define STR_MAX_LEN 254 // want sizeof(String) == mod 4

typedef struct String {
    unsigned len:8;
    unsigned type:2; // 2-bit, types: NOTIFY/TX/RCV/UNDEFINED
    char buf[STR_MAX_LEN]; // faster memcpy. sizeof(char) = 8-bits
} String;

static inline void sinit(String *s) {
    s->len = 0;
}

static inline unsigned slen(String *s) {
    return s->len;
}

static inline void sputc(String *s, const char c) {
    assert(s->len < STR_MAX_LEN);
    s->buf[s->len++] = c;
}

static inline void sputstr(String *s, const char *str) {
    for( ; *str != '\0' ; sputc(s, *(str++)) );
}

static inline void sputuint(String *s, int num, unsigned short base) {
    int dgt;
    unsigned int d = 1;

    while ((num / d) >= base) {
        d *= base;
    }

    while (d != 0) {
        dgt = num / d;
        num %= d;
        d /= base;

        sputc(s, (dgt < 10) ? dgt + '0' : (dgt - 10) + 'a');
    }
}

static inline void sputint(String *s, int num, unsigned short base) {
   if (num < 0) {
        sputc(s, '-');
        num *= -1;
    }

    sputuint(s, num, base);
}

static inline char *sbuf(String *s) {
    return s->buf;
}

static inline void sformat(String *s ) {
// TODO: if needed, make sformat (similar to printf)
// for now, just use sputc to manually build a string
    (void)s;
}


#endif
