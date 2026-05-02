#ifndef XELAGRAPH_H
#define XELAGRAPH_H

#pragma once
#include <stdint.h>
#include "types.h"


#define G_BLACK 0x00000000
#define G_BLUE 0x000000AA
#define G_GREEN 0x0000AA00
#define G_CYAN 0x0000AAAA
#define G_RED 0x00AA0000
#define G_MAGENTA 0x00AA00AA
#define G_BROWN 0x00AA5500
#define G_GREY 0x00AAAAAA
#define G_DARK_GREY 0x00555555
#define G_LIGHT_BLUE 0x005555FF
#define G_LIGHT_GREEN 0x0055FF55
#define G_LIGHT_CYAN 0x0055FFFF
#define G_LIGHT_RED 0x00FF5555
#define G_LIGHT_MAGENTA 0x00FF55FF
#define G_YELLOW 0x00FFFF55
#define G_WHITE  0x00FFFFFF

typedef struct 
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
} __attribute__((packed)) multiboot_info_t;

void fb_init(multiboot_info_t *mbi);
void drawpixel(uint32_t x, uint32_t y, uint32_t color);

void drawrectangle(uint32_t x, uint16_t y, uint32_t len_x, uint32_t len_y, uint32_t color);
void drawcircle(uint32_t cen_x, uint32_t cen_y, uint32_t r, uint32_t color);
void drawline(uint32_t ax, uint32_t ay, uint32_t bx, uint32_t by, uint32_t color);
void drawimage_indexed_scaled(int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t scale, const uint8_t *map, const uint32_t *palette);

void rtc_get_time(uint8_t *h, uint8_t *m, uint8_t *s);
void fb_write_u8_2digit(uint32_t *px, uint32_t py, uint8_t val, uint32_t fg, uint32_t bg);
void fb_write_str(uint32_t px, uint32_t py, const char *s, uint32_t fg, uint32_t bg);
void g_clean(void);
uint g_putc(char c, uint color, uint line);
uint g_write(const char *s, uint line, uint color);
uint g_scan(char *buf, uint max, uint line, const char *prompt);

void topbar_refresh(void);

void clearscreen(uint32_t color);

uint32_t fb_width(void);
uint32_t fb_height(void);
void fb_write_str(uint32_t px, uint32_t py, const char *s, uint32_t fg, uint32_t bg);

void cursor_set(uint32_t x, uint32_t y);

#endif