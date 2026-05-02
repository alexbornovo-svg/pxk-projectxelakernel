#include "idt.h"

struct idt_entry idt[256];
struct idt_ptr   idtp;

extern void keyboard_handler_stub();
extern void timer_handler_stub();

void idt_set_gate(uchar num, uint base, ushort sel, uchar flags) {
    idt[num].base_low  = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint)&idt;

    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_set_gate(32, (uint32_t)timer_handler_stub, 0x08, 0x8E);
    
    idt_set_gate(33, (uint)keyboard_handler_stub, 0x08, 0x8E);

    idt_load((uint)&idtp);
}