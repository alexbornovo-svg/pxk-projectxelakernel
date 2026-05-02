#ifndef ITALY_H
#define ITALY_H

/* Mappatura Scancode Set 1 - Layout Italiano */
unsigned char keyboard_map_it[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', 0x8D, /* 0x00 - 0x0D */
    '\b', /* Backspace */
    '\t', /* Tab */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0x8A, '+', '\n', /* Enter */
    0,    /* 0x1D - Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0xA2, 0x85, 0x97,
    0,    /* 0x2A - Left Shift */
    '<', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', 
    0,    /* 0x36 - Right Shift */
    '*',
    0,    /* Alt */
    ' ',  /* Space */
    0,    /* Caps lock */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F1-F10 */
    0,    /* Num lock */
    0,    /* Scroll lock */
    0,    /* Home key */
    0,    /* Up arrow */
    0,    /* Page up */
    '-',
    0,    /* Left arrow */
    0,
    0,    /* Right arrow */
    '+',
    0,    /* End key */
    0,    /* Down arrow */
    0,    /* Page down */
    0,    /* Insert key */
    0,    /* Delete key */
    0, 0, 0,
    0,    /* F11 */
    0,    /* F12 */
    0,    /* Tutto il resto è 0 */
};

#endif