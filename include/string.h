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

#define STR_MAX_LEN 253 // want sizeof(String) == mod 4

typedef struct String {
    unsigned len:8;
    unsigned type:2; // 2-bit, types: NOTIFY/TX/RCV/UNDEFINED
    char buf[STR_MAX_LEN + 1]; // faster memcpy. sizeof(char) = 8-bits
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



#define __va_argsiz(t)  \
        (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap)  ((void)0)

#define va_arg(ap, t)   \
         (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))


static inline int _sa2d( char ch ) {
    if( ch >= '0' && ch <= '9' ) return ch - '0';
    if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
    if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
    return -1;
}

static inline char _sa2i( char ch, char **src, int base, int *nump ) {
    int num, digit;
    char *p;

    p = *src; num = 0;
    while( ( digit = _sa2d( ch ) ) >= 0 ) {
        if ( digit > base ) break;
        num = num*base + digit;
        ch = *p++;
    }
    *src = p; *nump = num;
    return ch;
}

static inline void _sputw(String *s, int n, char fc, char *bf) {
    char ch;
    char *p = bf;

    while( *p++ && n > 0 ) n--;
    while( n-- > 0 ) sputc(s, fc);
    while( ( ch = *bf++ ) ) sputc(s, ch);
}

static inline void _sui2a(unsigned int num, unsigned int base, char *bf) {
    int n = 0;
    int dgt;
    unsigned int d = 1;

    while( (num / d) >= base ) d *= base;
    while( d != 0 ) {
        dgt = num / d;
        num %= d;
        d /= base;
        if( n || dgt > 0 || d == 0 ) {
            *bf++ = dgt + ( dgt < 10 ? '0' : 'a' - 10 );
            ++n;
        }
    }
    *bf = 0;
}


static inline void _si2a( int num, char *bf ) {
    if( num < 0 ) {
        num = -num;
        *bf++ = '-';
    }
    _sui2a( num, 10, bf );
}


static inline void sformat(String *s, char *fmt, va_list va) {
    char bf[12];
    char ch, lz;
    int w;

    while ( ( ch = *(fmt++) ) ) {
        if ( ch != '%' )
            sputc(s, ch );
        else {
            lz = 0; w = 0;
            ch = *(fmt++);
            switch ( ch ) {
            case '0':
                lz = 1; ch = *(fmt++);
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                ch = _sa2i( ch, &fmt, 10, &w );
                break;
            }
            switch( ch ) {
            case 0: return;
            case 'c':
                sputc(s, va_arg( va, char ) );
                break;
            case 's':
                _sputw(s, w, 0, va_arg( va, char* ));
                break;
            case 'u':
                _sui2a(va_arg( va, unsigned int ), 10, bf );
                _sputw(s, w, lz, bf );
                break;
            case 'd':
                _si2a(va_arg( va, int ), bf);
                _sputw(s, w, lz, bf);
                break;
            case 'x':
                _sui2a(va_arg( va, unsigned int ), 16, bf);
                _sputw(s, w, lz, bf);
                break;
            case '%':
                sputc(s, ch);
                break;
            }
        }
    }
}


static inline void sprintf(String *s, char *fmt, ... ) {
    va_list va;
    va_start(va, fmt);
    sformat(s, fmt, va );
    va_end(va);
}


#endif
