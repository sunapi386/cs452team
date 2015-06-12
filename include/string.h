#ifndef __STRING_H
#define __STRING_H
#include <debug.h>

#define STR_MAX_LEN 63 // want sizeof(String) == mod 4

typedef struct String {
    unsigned len;
    char buf[STR_MAX_LEN]; // faster memcpy
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
    for( ; *str != '\0' ; sputc(s, str++) );
}

static inline void sputuint(String *s, const int u, unsigned short base) {
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

static inline void sputint(String *s, const int i) {
   if (num < 0) {
        sputc(s, '-');
        num *= -1;
    }

    sputuint(s, num, base);
}



#endif
