#ifndef GDT_H
#define GDT_H

#include "types.h"

struct gdt 
{
    unsigned int address;
    unsigned short size;
} __attribute__((packed));

struct gdt_entry {
    ushort limit_low, base_low;
    uchar  base_mid, access, granularity, base_high;
} __attribute__((packed));

struct gdt_ptr {
    ushort limit;
    uint   base;
} __attribute__((packed));

void gdt_init();
void gdt_load(uint);

#endif