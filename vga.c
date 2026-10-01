#include "vga.h"
#include "serial.h"

static size_t vga_row;
static size_t vga_column;
static uint8_t vga_color;
static uint16_t* vga_buffer;
static size_t prompt_min_column;
static size_t prompt_min_row;

void vga_init(void) {
    serial_init();
    vga_row = 0;
    vga_column = 0;
    vga_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_buffer = VGA_MEMORY;
    prompt_min_column = 0;
    prompt_min_row = 0;
    vga_clear();
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_color = vga_entry_color(fg, bg);
}

uint8_t vga_get_color(void) {
    return vga_color;
}

void vga_update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', vga_color);
        }
    }
    vga_row = 0;
    vga_column = 0;
    vga_update_cursor(0, 0);
}

static void vga_scroll(void) {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', vga_color);
    }
    vga_row = VGA_HEIGHT - 1;
    if (prompt_min_row > 0) prompt_min_row--;
}

void vga_putchar(char c) {
    serial_putchar(c);

    if (c == '\n') {
        vga_column = 0;
        if (++vga_row == VGA_HEIGHT) {
            vga_scroll();
        }
    } else if (c == '\r') {
        vga_column = 0;
    } else if (c == '\t') {
        vga_column = (vga_column + 4) & ~3;
        if (vga_column >= VGA_WIDTH) {
            vga_column = 0;
            if (++vga_row == VGA_HEIGHT) {
                vga_scroll();
            }
        }
    } else if (c == '\b') {
        vga_backspace();
    } else {
        const size_t index = vga_row * VGA_WIDTH + vga_column;
        vga_buffer[index] = vga_entry(c, vga_color);
        if (++vga_column == VGA_WIDTH) {
            vga_column = 0;
            if (++vga_row == VGA_HEIGHT) {
                vga_scroll();
            }
        }
    }
    vga_update_cursor(vga_column, vga_row);
}

void vga_set_prompt_boundary(void) {
    prompt_min_row = vga_row;
    prompt_min_column = vga_column;
}

void vga_backspace(void) {
    if (vga_row < prompt_min_row) return;
    if (vga_row == prompt_min_row && vga_column <= prompt_min_column) return;

    if (vga_column > 0) {
        vga_column--;
    } else if (vga_row > prompt_min_row) {
        vga_row--;
        vga_column = VGA_WIDTH - 1;
    } else {
        return;
    }

    const size_t index = vga_row * VGA_WIDTH + vga_column;
    vga_buffer[index] = vga_entry(' ', vga_color);
    vga_update_cursor(vga_column, vga_row);
}

void vga_putstr(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}

void vga_putdec(int n) {
    char buf[32];
    itoa(n, buf, 10);
    vga_putstr(buf);
}

void vga_puthex(uint32_t n) {
    char buf[32];
    itoa((int)n, buf, 16);
    vga_putstr("0x");
    vga_putstr(buf);
}
