#ifndef __UTILS_H
#define __UTILS_H

typedef unsigned int size_t;

void memcpy(void *dest, const void *src, size_t n);
int strcmp (const char * dst, const char * src);
char * strncpy(char *dst, const char *src, size_t n);

#endif
