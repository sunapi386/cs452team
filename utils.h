#ifndef __UTILS_H
#define __UTILS_H

typedef unsigned int size_t;

void memcpy(void *dest, const void *src, size_t n);
int strcmp(char *s1, char *s2);
char * strncpy(char *dst, const char *src, size_t n);

#endif
