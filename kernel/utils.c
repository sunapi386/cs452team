#include <va.h>
#include <utils.h>
#include <debug.h>
#include <user/syscall.h>

// NOTE: Insecure (does not detect overlapping memory)
void memcpy(void *dest, const void *src, size_t n) {
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

void scopy(String *dst, const char *src) {
    sinit(dst);
    size_t n = strlen(src);
    assert(n < STR_MAX_LEN);
    strncpy(dst->buf, src, n);
}

void scopystr(String *dst, String *src) {
    sinit(dst);
    size_t n = src->len;
    strncpy(dst->buf, src->buf, n);
}

void sputc(String *s, const char c) {
    assert(s->len < STR_MAX_LEN);
    s->buf[s->len++] = c;
}

void sputstr(String *s, const char *str) {
    for( ; *str != '\0' ; sputc(s, *(str++)) );
}

void sconcat(String *dst, String *src) {
    assert(dst->len + src->len < STR_MAX_LEN);
    for(unsigned i = 0; i < src->len; i++) {
        sputc(dst, src->buf[i]);
    }
}

void sputuint(String *s, int num, unsigned short base) {
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

void sputint(String *s, int num, unsigned short base) {
   if (num < 0) {
        sputc(s, '-');
        num *= -1;
    }

    sputuint(s, num, base);
}

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

void sformat(String *s, char *fmt, va_list va) {
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

void sprintf(String *s, char *fmt, ... ) {
    va_list va;
    va_start(va, fmt);
    sformat(s, fmt, va );
    va_end(va);
}

void printf(int channel, char *fmt, ... ) {
    String s;
    sinit(&s);

    va_list va;
    va_start(va, fmt);
    sformat(&s, fmt, va);
    va_end(va);

    PutString(channel, &s);
}
