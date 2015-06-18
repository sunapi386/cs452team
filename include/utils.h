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
    char *data;
    size_t size, head, tail;
} CBuffer;

void CBufferInit(CBuffer *b, char * array, size_t size);
int CBufferPush(CBuffer *b, char ch);
char CBufferPop(CBuffer *b);
bool CBufferIsEmpty(const CBuffer *b);
void CBufferClean(CBuffer *b);

/**
CBufferPushStr: pushes a NUL terminated char array into a CBuffer

Return values
    -1: overflow occured
    or non-negative integer for the number of characters pushed
*/
int CBufferPushStr(CBuffer *b, char *str);

#endif
