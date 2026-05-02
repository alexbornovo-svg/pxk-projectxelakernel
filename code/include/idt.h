#ifndef IDT_H
#define IDT_H

#include "types.h"

struct idt_entry {
    ushort base_low;
    ushort sel;
    uchar  always0;
    uchar  flags;
    ushort base_high; 
} __attribute__((packed));

struct idt_ptr {
    ushort limit;
    uint   base;
} __attribute__((packed));

void idt_init();
extern void idt_load(uint);

#endif