#include "string.h"
#include "types.h"

uint32_t strlen(const char *s)
{
    uint32_t i = 0;
    while (s[i] != '\0')
        i++;
    return i;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) 
    {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, uint32_t n)
{
    while (n && *a && *a == *b) 
    {
        a++;
        b++;
        n--;
    }
    if (n == 0)
        return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *src++))
        ;
    return ret;
}

char *strncpy(char *dst, const char *src, uint32_t n)
{
    char *ret = dst;
    while (n && (*dst++ = *src++))
        n--;
    while (n--)
        *dst++ = '\0';
    return ret;
}

char *strcat(char *dst, const char *src)
{
    char *ret = dst;
    while (*dst)
        dst++;
    while ((*dst++ = *src++))
        ;
    return ret;
}

char *strchr(const char *s, char c)
{
    while (*s) 
    {
        if (*s == c)
            return (char *)s;
        s++;
    }
    return 0;
}

void *memcpy(void *dst, const void *src, uint32_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--)
        *d++ = *s++;
    return dst;
}

void *memset(void *dst, uint8_t val, uint32_t n)
{
    uint8_t *d = (uint8_t *)dst;
    while (n--)
        *d++ = val;
    return dst;
}

int memcmp(const void *a, const void *b, uint32_t n)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    while (n--) 
    {
        if (*pa != *pb)
            return *pa - *pb;
        pa++;
        pb++;
    }
    return 0;
}

void *memmove(void *dst, const void *src, uint32_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) 
    {
        while (n--)
            *d++ = *s++;
    } 
    else 
    {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dst;
}