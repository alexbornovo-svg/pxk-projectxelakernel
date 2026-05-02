#include "io.h"
#include "types.h"
#include "kstd.h"
#include "../drivers/kbd.h"

static int cursor_x = 0;
static int cursor_y = 0;

void vga_scroll()
{
    volatile char *vidmem = (volatile char *)0xb8000;
    for (int i = 0; i < 24 * 80 * 2; i++)
        vidmem[i] = vidmem[i + 80 * 2];
    for (int i = 24 * 80 * 2; i < 25 * 80 * 2; i += 2) 
    {
        vidmem[i]     = ' ';
        vidmem[i + 1] = 0x0F;
    }
    cursor_y = 24;
}

void vga_clean()
{
    volatile char *vidmem = (volatile char *)0xb8000;
    for (unsigned int i = 0; i < 80 * 25 * 2; i += 2) 
    {
        vidmem[i]     = ' ';
        vidmem[i + 1] = 0x0F;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_hardware_cursor();
}

uint vga_write(const char *message, uint line, uchar color)
{
    volatile char *vidmem = (volatile char *)0xb8000;
    unsigned int i = line * 80 * 2;
    while (*message != 0) 
    {
        if (*message == '\n') 
        {
            line++;
            i = line * 80 * 2;
        } 
        else 
        {
            vidmem[i]     = *message;
            vidmem[i + 1] = color;
            i += 2;
        }
        message++;
    }
    return line + 1;
}

uint vga_putc(char c, uchar color, uint line)
{
    volatile char *vidmem = (volatile char *)0xb8000;
    if (c == '\n') 
    {
        cursor_x = 0;
        cursor_y++;
    } 
    else if (c == '\b') 
    {
        if (cursor_x > 0) 
        {
            cursor_x--;
        } 
        else if (cursor_y > 0) 
        {
            cursor_y--;
            cursor_x = 79;
        }
        int pos = (cursor_y * 80 + cursor_x) * 2;
        vidmem[pos]     = ' ';
        vidmem[pos + 1] = color;
    } 
    else 
    {
        int pos = (cursor_y * 80 + cursor_x) * 2;
        vidmem[pos]     = c;
        vidmem[pos + 1] = color;
        cursor_x++;
        if (cursor_x >= 80) 
        {
            cursor_x = 0;
            cursor_y++;
        }
    }
    if (cursor_y >= 25)
    {
        vga_scroll();
        line--;
    }
    update_hardware_cursor();
    return line;
}

void update_hardware_cursor()
{
    unsigned short pos = cursor_y * 80 + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

char get_char()
{
    char c;
    while ((c = kbd_dequeue()) == 0)
        asm volatile("hlt");
    return c;
}

uint vga_scan(char *buffer, int max_len, uint line, char *prompt)
{
    if (cursor_y > (int)line)
        line = cursor_y;

    if (prompt != 0) 
    {
        line = vga_write(prompt, line, 0x0F);
        cursor_y = line - 1;
        cursor_x = 0;
        for (int i = 0; prompt[i] != 0; i++)
        {
            cursor_x++;
        }
        update_hardware_cursor();
    }
    int i = 0;
    while (i < max_len - 1) 
    {
        char c = get_char();
        if (c == '\n') 
        {
            buffer[i] = '\0';
            line = vga_putc('\n', 0x0F, line);
            break;
        } 
        else if (c == '\b') 
        {
            if (i > 0) 
            {
                i--;
                vga_putc('\b', 0x0F, line);
            }
        } 
        else if (c >= 32 && c <= 126) 
        {
            buffer[i++] = c;
            vga_putc(c, 0x0F, line);
        }
    }
    if (i == max_len - 1)
    {
        buffer[i] = '\0';
    }
    return line;
}

uint k_scanf(char *buffer, int max_len, uint line, char *prompt)
{
    line = vga_scan(buffer, max_len, line, prompt);
    return line;
}

uint vga_get_cursor_y()
{
    return cursor_y;
}

uint vga_get_cursor_x()
{
    return cursor_x;
}