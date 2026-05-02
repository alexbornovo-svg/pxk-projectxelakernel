#include "xelagraph.h"
#include "types.h"
#include "math.h"
#include "io.h"
#include "../lib/kstd.h"
#include "../misc/font88.h"
#include "../drivers/kbd.h"

#define CHAR_W   8
#define CHAR_H   8
#define SCALE    2
#define LINE_GAP 6
#define CELL_H   (CHAR_H * SCALE + LINE_GAP)
#define COLS     (800 / (CHAR_W * SCALE))
#define ROWS     (450 / CELL_H)

static uint8_t  *fb       = 0;
static uint32_t  fb_pitch = 0;
static uint32_t  fb_w     = 0;
static uint32_t  fb_h     = 0;
static uint8_t   fb_bpp   = 0;

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;

static const char *topbar_cwd = "/";

static uint32_t color_map[] = {
    0x000000,  // 0 BLACK
    0x888888,  // 1 GREY
    0xFFFFFF,  // 2 WHITE
    0xFF3333,  // 3 RED
    0x33FF33,  // 4 GREEN
    0x3333FF,  // 5 BLUE
    0x33FFFF,  // 6 CYAN
    0xFFFF33,  // 7 YELLOW
};

// ── RTC ──────────────────────────────────────────────────────────────────────

static uint8_t rtc_read(uint8_t reg)
{
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd_to_bin(uint8_t v)
{
    return (v & 0x0F) + ((v >> 4) * 10);
}

void rtc_get_time(uint8_t *h, uint8_t *m, uint8_t *s)
{
    *h = bcd_to_bin(rtc_read(0x04));
    *m = bcd_to_bin(rtc_read(0x02));
    *s = bcd_to_bin(rtc_read(0x00));
}

// ── FRAMEBUFFER INIT ─────────────────────────────────────────────────────────

void fb_init(multiboot_info_t *mbi)
{
    fb       = (uint8_t *)(uint32_t)mbi->framebuffer_addr;
    fb_pitch = mbi->framebuffer_pitch;
    fb_w     = mbi->framebuffer_width;
    fb_h     = mbi->framebuffer_height;
    fb_bpp   = mbi->framebuffer_bpp;
}

uint32_t fb_width(void)  { return fb_w; }
uint32_t fb_height(void) { return fb_h; }

// ── DRAW PRIMITIVES ──────────────────────────────────────────────────────────

void drawpixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (!fb || x >= fb_w || y >= fb_h) return;
    uint32_t *p = (uint32_t *)(fb + y * fb_pitch + x * (fb_bpp / 8));
    *p = color;
}

void drawrectangle(uint32_t x, uint16_t y, uint32_t len_x, uint32_t len_y, uint32_t color)
{
    for (uint32_t i = 0; i < len_x; i++)
        for (uint32_t j = 0; j < len_y; j++)
            drawpixel(x + i, y + j, color);
}

void drawline(uint32_t ax, uint32_t ay, uint32_t bx, uint32_t by, uint32_t color)
{
    int dx =  absval(ax - bx);
    int dy = -absval(ay - by);
    int sx = ax < bx ? 1 : -1;
    int sy = ay < by ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        drawpixel(ax, ay, color);
        if (ax == bx && ay == by) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; ax += sx; }
        if (e2 <= dx) { err += dx; ay += sy; }
    }
}

void drawcircle(uint32_t cen_x, uint32_t cen_y, uint32_t r, uint32_t color)
{
    int x = 0, y = (int)r, d = (int)r - 1;
    while (y >= x) {
        drawrectangle(cen_x - x, cen_y + y, 2 * x + 1, 1, color);
        drawrectangle(cen_x - x, cen_y - y, 2 * x + 1, 1, color);
        drawrectangle(cen_x - y, cen_y + x, 2 * y + 1, 1, color);
        drawrectangle(cen_x - y, cen_y - x, 2 * y + 1, 1, color);
        if (d >= 2 * x)        { d -= 2 * x + 1; x++; }
        else if (d < 2 * ((int)r - y)) { d += 2 * y - 1; y--; }
        else                   { d += 2 * (y - x - 1); y--; x++; }
    }
}

void clearscreen(uint32_t color)
{
    drawrectangle(0, 0, fb_w, fb_h, color);
}

void drawimage_indexed_scaled(int32_t x, int32_t y, uint32_t w, uint32_t h,
                               uint32_t scale, const uint8_t *map, const uint32_t *palette)
{
    if (scale == 0) scale = 1;
    for (uint32_t i = 0; i < h; i++)
        for (uint32_t j = 0; j < w; j++) {
            uint32_t color = palette[map[i * w + j]];
            drawrectangle(x + j * scale, y + i * scale, scale, scale, color);
        }
}

// ── FONT / TEXT ──────────────────────────────────────────────────────────────

void fb_draw_char(uint32_t cx, uint32_t cy, char c, uint32_t color)
{
    if ((uint8_t)c >= 128) return;
    const uint8_t *glyph = font8x8[(uint8_t)c];
    uint32_t px = cx * CHAR_W * SCALE;
    uint32_t py = cy * CELL_H;
    for (int row = 0; row < CHAR_H; row++)
        for (int col = 0; col < CHAR_W; col++) {
            uint32_t pcolor = (glyph[row] & (0x80 >> col)) ? color : 0x000000;
            for (int sy = 0; sy < SCALE; sy++)
                for (int sx = 0; sx < SCALE; sx++)
                    drawpixel(px + col*SCALE + sx, py + row*SCALE + sy, pcolor);
        }
}

void fb_write_str(uint32_t px, uint32_t py, const char *s, uint32_t fg, uint32_t bg)
{
    uint32_t orig_cx = cursor_x, orig_cy = cursor_y;
    uint32_t x = px;
    for (int i = 0; s[i]; i++) {
        if ((uint8_t)s[i] >= 128) { x += CHAR_W * SCALE; continue; }
        const uint8_t *glyph = font8x8[(uint8_t)s[i]];
        for (int row = 0; row < CHAR_H; row++)
            for (int col = 0; col < CHAR_W; col++) {
                uint32_t color = (glyph[row] & (0x80 >> col)) ? fg : bg;
                for (int sy = 0; sy < SCALE; sy++)
                    for (int sx = 0; sx < SCALE; sx++)
                        drawpixel(x + col*SCALE + sx, py + row*SCALE + sy, color);
            }
        x += CHAR_W * SCALE;
    }
    cursor_x = orig_cx;
    cursor_y = orig_cy;
}

void fb_write_u8_2digit(uint32_t *px, uint32_t py, uint8_t val, uint32_t fg, uint32_t bg)
{
    char buf[3];
    buf[0] = '0' + val / 10;
    buf[1] = '0' + val % 10;
    buf[2] = '\0';
    fb_write_str(*px, py, buf, fg, bg);
    *px += CHAR_W * SCALE * 2;
}

// ── TOPBAR ───────────────────────────────────────────────────────────────────

void topbar_set_cwd(const char *cwd) { topbar_cwd = cwd; }

void topbar_refresh(void)
{
    uint32_t cx = cursor_x, cy = cursor_y;
    uint8_t h, m, s;
    rtc_get_time(&h, &m, &s);

    drawrectangle(0, 0, fb_w, 20, 0x222222);
    fb_write_str(4, 2, "PXK2", 0x33FFFF, 0x222222);
    fb_write_str(fb_w / 2 - 40, 2, topbar_cwd, 0xAAAAAA, 0x222222);

    uint32_t tx = fb_w - (CHAR_W * SCALE * 8) - 4;
    fb_write_u8_2digit(&tx, 2, h, 0xFFFFFF, 0x222222);
    fb_write_str(tx, 2, ":", 0xFFFFFF, 0x222222); tx += CHAR_W * SCALE;
    fb_write_u8_2digit(&tx, 2, m, 0xFFFFFF, 0x222222);
    fb_write_str(tx, 2, ":", 0xFFFFFF, 0x222222); tx += CHAR_W * SCALE;
    fb_write_u8_2digit(&tx, 2, s, 0xFFFFFF, 0x222222);

    cursor_x = cx;
    cursor_y = cy;
}

// ── SCROLL ───────────────────────────────────────────────────────────────────

void fb_scroll(void)
{
    for (uint32_t y = 0; y < (ROWS - 1) * CELL_H; y++)
        for (uint32_t x = 0; x < fb_w * (fb_bpp / 8); x++)
            fb[y * fb_pitch + x] = fb[(y + CELL_H) * fb_pitch + x];
    for (uint32_t y = (ROWS - 1) * CELL_H; y < ROWS * CELL_H; y++)
        for (uint32_t x = 0; x < fb_w * (fb_bpp / 8); x++)
            fb[y * fb_pitch + x] = 0;
}

// ── VGA-STYLE OUTPUT ─────────────────────────────────────────────────────────

void g_clean(void)
{
    clearscreen(0x000000);
    cursor_x = 0;
    cursor_y = 0;
}

uint g_putc(char c, uint color, uint line)
{
    (void)line;
    uint32_t fg = (color < 8) ? color_map[color] : 0xFFFFFF;
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        fb_draw_char(cursor_x, cursor_y, c, fg);
        cursor_x++;
        if (cursor_x >= COLS) { cursor_x = 0; cursor_y++; }
    }
    if (cursor_y >= ROWS) { fb_scroll(); cursor_y = ROWS - 1; }
    return cursor_y;
}

uint g_write(const char *s, uint line, uint color)
{
    for (int i = 0; s[i]; i++) g_putc(s[i], color, line);
    g_putc('\n', color, line);
    return cursor_y;
}

static char kbd_getc(void)
{
    char c;
    while ((c = kbd_dequeue()) == 0);
    return c;
}

uint g_scan(char *buf, uint max, uint line, const char *prompt)
{
    for (int i = 0; prompt[i]; i++) g_putc(prompt[i], 2, line);
    uint pos = 0;
    while (1) {
        char c = kbd_getc();
        if (c == '\n' || c == '\r') {
            buf[pos] = '\0';
            g_putc('\n', 2, line);
            break;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                if (cursor_x > 0) cursor_x--;
                else { cursor_y--; cursor_x = COLS - 1; }
                fb_draw_char(cursor_x, cursor_y, ' ', 0x000000);
            }
        } else if (c >= 32 && pos < max - 1) {
            buf[pos++] = c;
            g_putc(c, 2, line);
        }
    }
    return cursor_y;
}

void cursor_set(uint32_t x, uint32_t y) 
{ 
    cursor_x = x; 
    cursor_y = y; 
}