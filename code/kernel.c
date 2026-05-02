#include "idt.h"
#include "../lib/kstd.h"
#include "../lib/pic.h"
#include "../lib/gdt.h"
#include "../drivers/ata.h"
#include "../drivers/ext2.h"
#include "../programs/shell.h"

void kmain()
{
    uint line = 0;
    vga_clean();
    line = vga_write("GDT initialization...", line, GREY);
    gdt_init();
    line = vga_write("PIC initialization...", line, GREY);
    pic_remap();
    line = vga_write("IDT initialization...", line, GREY);
    idt_init();
    asm volatile("sti");

    line = vga_write("ATA initialization...", line, GREY);
    ata_init();
    if (!ata_is_ready())
        line = vga_write("ATA: disk not founded", line, RED);
    else
    {
        line = vga_write("ATA: disk founded", line, GREY);
        if (ext2_init(0) < 0) line = vga_write("EXT2: error during mount", line, RED);
        else line = vga_write("EXT2: mount ok", line, GREY);
    }

    line = vga_write("PX KERNEL 2 READY", line, GREEN);
    line++;

    shell_run(line);
}