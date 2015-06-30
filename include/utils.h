#ifndef __UTILS_H
#define __UTILS_H

typedef unsigned int size_t;
typedef enum { false, true } bool;

void memcpy(void *dest, const void *src, size_t n);
int strcmp (const char * dst, const char * src);
char * strncpy(char *dst, const char *src, size_t n);
size_t strlen(const char *str); // does not count null terminator
int countLeadingZeroes(const unsigned int mask);


typedef struct CBuffer {
    char *data;
    size_t size, head, tail;
} CBuffer;

void CBufferInit(CBuffer *b, char * array, size_t size);
int CBufferPush(CBuffer *b, char ch);
char CBufferPop(CBuffer *b);
bool CBufferIsEmpty(const CBuffer *b);
void CBufferClean(CBuffer *b);
int CBufferPushStr(CBuffer *b, char *str);

#define STR_MAX_LEN 1024

typedef struct String {
    unsigned int len;
    char buf[STR_MAX_LEN + 1];
} String;

static inline void sinit(String *s) {
    s->len = 0;
}
static inline unsigned int slen(String *s) {
    return s->len;
}
void scopy(String *dst, const char *src);
void scopystr(String *dst, String *src);
void sputc(String *s, const char c);
void sputstr(String *s, const char *str);
void sconcat(String *dst, String *src);
void sputuint(String *s, int num, unsigned short base);
void sputint(String *s, int num, unsigned short base);
void sformat(String *s, char *fmt, char * va);
void sprintf(String *s, char *fmt, ... );
void sprintfstr(String *s, char *fmt, ... );
void printf(int channel, char *fmt, ... );

#endif
