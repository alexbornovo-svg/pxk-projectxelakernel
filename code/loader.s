global loader
extern kmain
extern multiboot_mem_lower
extern multiboot_mem_upper
global idt_load
extern keyboard_handler_c

MAGIC_NUMBER equ 0x1BADB002
FLAGS        equ 0x0
CHECKSUM     equ -(MAGIC_NUMBER + FLAGS)

section .multiboot
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

section .text

loader:
    mov esp, stack_top

    mov eax, [ebx + 4]
    mov [multiboot_mem_lower], eax
    mov eax, [ebx + 8]
    mov [multiboot_mem_upper], eax

    call kmain

.loop:
    hlt
    jmp .loop

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

global keyboard_handler_stub
keyboard_handler_stub:
    pushad
    call keyboard_handler_c
    popad
    iretd

global gdt_load
gdt_load:
    mov eax, [esp+4]
    lgdt [eax]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush
.flush:
    ret

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: