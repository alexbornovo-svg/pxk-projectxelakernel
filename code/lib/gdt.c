#include "gdt.h"
#include "types.h"

struct gdt_entry gdt[3];
struct gdt_ptr   gdtp;

static void gdt_set(int i, uint base, uint limit, uchar access, uchar gran) {
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].base_high   = (base >> 24) & 0xFF;
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access      = access;
}

void gdt_init() {
    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base  = (uint)&gdt;

    gdt_set(0, 0, 0,          0,    0);     // null
    gdt_set(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // code  → CS = 0x08
    gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // data  → DS = 0x10

    gdt_load((uint)&gdtp);  // in loader.s
}