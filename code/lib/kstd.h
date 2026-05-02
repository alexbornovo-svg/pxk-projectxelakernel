#ifndef KSTD_H
#define KSTD_H

#include "types.h"

#define BLACK 0x00
#define GREEN 0x02
#define RED 0x04
#define GREY 0x07
#define CYAN 0x0B
#define YELLOW 0x0E
#define WHITE 0x0F

void vga_clean();
uint vga_write(const char *message, uint line, uchar color);
uint vga_putc(char c, uchar color, uint line);
uint vga_scan(char *buffer, int max_len, uint line, char *prompt);
void update_hardware_cursor();
char get_char();
uint k_scanf(char *buffer, int max_len, uint line, char *prompt);
uint vga_get_cursor_y();
uint vga_get_cursor_x();

#endif