#ifndef STRING_H
#define STRING_H

#include "types.h"

uint32_t strlen(const char *s);
int      strcmp(const char *a, const char *b);
int      strncmp(const char *a, const char *b, uint32_t n);
char    *strcpy(char *dst, const char *src);
char    *strncpy(char *dst, const char *src, uint32_t n);
char    *strcat(char *dst, const char *src);
char    *strchr(const char *s, char c);

void    *memcpy(void *dst, const void *src, uint32_t n);
void    *memset(void *dst, uint8_t val, uint32_t n);
int      memcmp(const void *a, const void *b, uint32_t n);
void    *memmove(void *dst, const void *src, uint32_t n);

#endif