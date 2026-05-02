#ifndef IO_H
#define IO_H

#include "types.h"

static inline void outw(ushort port, uint16_t value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(ushort port)
{
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(ushort port, uchar data)
{
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uchar inb(ushort port)
{
    uchar result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

#endif