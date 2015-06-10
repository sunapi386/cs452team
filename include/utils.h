#ifndef __UTILS_H
#define __UTILS_H

typedef unsigned int size_t;
typedef enum { false, true } bool;

void memcpy(void *dest, const void *src, size_t n);
int strcmp (const char * dst, const char * src);
char * strncpy(char *dst, const char *src, size_t n);
size_t strlen(const char *str);
int countLeadingZeroes(const unsigned int mask);


typedef struct CBuffer {
    void *data;
    int size;
} CBuffer;

void CBufferInit(CBuffer *b, void *array, size_t size);
void *CBufferPop(CBuffer *b);
int CBufferPush(CBuffer *b, void *item);
bool CBufferIsEmpty(CBuffer *b);
void CBufferClean(CBuffer *b);

#endif
