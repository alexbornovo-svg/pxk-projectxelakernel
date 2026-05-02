#include "io.h"
#include "types.h"
#include "italy.h"
#include "../drivers/kbd.h"

#define KBD_DATA_PORT   0x60
#define KBD_BUFFER_SIZE 256

static char kbd_buffer[KBD_BUFFER_SIZE];
static int  kbd_buffer_head = 0;
static int  kbd_buffer_tail = 0;

static void kbd_enqueue(char c)
{
    int next = (kbd_buffer_head + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_buffer_tail) {
        kbd_buffer[kbd_buffer_head] = c;
        kbd_buffer_head = next;
    }
}

char kbd_dequeue()
{
    if (kbd_buffer_head == kbd_buffer_tail) return 0;
    char c = kbd_buffer[kbd_buffer_tail];
    kbd_buffer_tail = (kbd_buffer_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

static uchar sc_to_ascii(uchar scancode)
{
    if (scancode & 0x80) return 0;
    return keyboard_map_it[scancode];
}

void keyboard_handler_c()
{
    uchar scancode = inb(KBD_DATA_PORT);

    if (!(scancode & 0x80)) 
    {
        uchar c = sc_to_ascii(scancode);
        if (c > 0)
            kbd_enqueue(c);
    }

    outb(0x20, 0x20); 
}