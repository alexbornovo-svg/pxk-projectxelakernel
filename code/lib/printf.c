#include "io.h"
#include "types.h"
#include "kstd.h"

static void itoa(int value, char *buf, int base) {
    char tmp[32];
    int i = 0;
    int negative = 0;

    if (value == 0) { buf[0] = '0'; buf[1] = '\0'; return; }

    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }

    unsigned int uval = (unsigned int)value;
    while (uval > 0) {
        int rem = uval % base;
        tmp[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
        uval /= base;
    }

    int j = 0;
    if (negative) buf[j++] = '-';
    for (int k = i - 1; k >= 0; k--)
        buf[j++] = tmp[k];
    buf[j] = '\0';
}

static void utoa(unsigned int value, char *buf, int base) {
    char tmp[32];
    int i = 0;

    if (value == 0) { buf[0] = '0'; buf[1] = '\0'; return; }

    while (value > 0) {
        int rem = value % base;
        tmp[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
        value /= base;
    }

    for (int k = i - 1, j = 0; k >= 0; k--, j++)
        buf[j] = tmp[k];
    buf[i] = '\0';
}

static uint puts_vga(const char *s, uchar color, uint line) {
    while (*s) {
        line = vga_putc(*s, color, line);
        s++;
    }
    return line;
}

uint printf(uchar color, uint line, const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    char buf[32];

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's': {
                    const char *s = __builtin_va_arg(args, const char *);
                    if (!s) s = "(null)";
                    line = puts_vga(s, color, line);
                    break;
                }
                case 'd': {
                    int val = __builtin_va_arg(args, int);
                    itoa(val, buf, 10);
                    line = puts_vga(buf, color, line);
                    break;
                }
                case 'u': {
                    unsigned int val = __builtin_va_arg(args, unsigned int);
                    utoa(val, buf, 10);
                    line = puts_vga(buf, color, line);
                    break;
                }
                case 'x': {
                    unsigned int val = __builtin_va_arg(args, unsigned int);
                    utoa(val, buf, 16);
                    line = puts_vga(buf, color, line);
                    break;
                }
                case 'X': {
                    unsigned int val = __builtin_va_arg(args, unsigned int);
                    utoa(val, buf, 16);
                    for (int i = 0; buf[i]; i++)
                        if (buf[i] >= 'a' && buf[i] <= 'f')
                            buf[i] -= 32;
                    line = puts_vga(buf, color, line);
                    break;
                }
                case 'c': {
                    char c = (char)__builtin_va_arg(args, int);
                    line = vga_putc(c, color, line);
                    break;
                }
                case '%': {
                    line = vga_putc('%', color, line);
                    break;
                }
                default:
                    line = vga_putc('%', color, line);
                    line = vga_putc(*fmt, color, line);
                    break;
            }
        } else {
            line = vga_putc(*fmt, color, line);
        }
        fmt++;
    }

    __builtin_va_end(args);
    return line;
}