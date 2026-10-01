#include "shell.h"
#include "kernel.h"
#include "vga.h"
#include "keyboard.h"

#define MAX_CMD_LEN 256

static char cmd_buf[MAX_CMD_LEN];
static size_t cmd_idx = 0;

static void print_prompt(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_putstr("foamyos");
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr("# ");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_set_prompt_boundary();
}

static uint32_t parse_num(const char* str) {
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        uint32_t val = 0;
        str += 2;
        while (*str) {
            char c = *str++;
            val <<= 4;
            if (c >= '0' && c <= '9') val |= (c - '0');
            else if (c >= 'a' && c <= 'f') val |= (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') val |= (c - 'A' + 10);
        }
        return val;
    } else {
        uint32_t val = 0;
        while (*str >= '0' && *str <= '9') {
            val = val * 10 + (*str++ - '0');
        }
        return val;
    }
}

static void execute_command(char* cmd) {
    /* Strip leading spaces */
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    /* Extract binary name and argument */
    char name[64];
    size_t i = 0;
    while (cmd[i] != '\0' && cmd[i] != ' ' && i < 63) {
        name[i] = cmd[i];
        i++;
    }
    name[i] = '\0';

    char* arg = &cmd[i];
    while (*arg == ' ') arg++;

    if (strcmp(name, "help") == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_putstr("FoamyOS Shell Commands:\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_putstr("  help            - Display available commands\n");
        vga_putstr("  clear           - Clear VGA screen\n");
        vga_putstr("  echo <text>     - Output arguments to terminal\n");
        vga_putstr("  sysinfo / fetch - Show operating system & hardware state\n");
        vga_putstr("  color <fg> <bg> - Change terminal foreground & background color (0..15)\n");
        vga_putstr("  peek <addr>     - Read byte at physical memory address\n");
        vga_putstr("  poke <addr> <val>- Write byte to physical memory address\n");
        vga_putstr("  panic           - Trigger test kernel panic exception\n");
        vga_putstr("  reboot          - Reset and reboot machine\n");
    }
    else if (strcmp(name, "clear") == 0) {
        vga_clear();
    }
    else if (strcmp(name, "echo") == 0) {
        vga_putstr(arg);
        vga_putchar('\n');
    }
    else if (strcmp(name, "sysinfo") == 0 || strcmp(name, "fetch") == 0) {
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_putstr("       /\\        OS: ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_putstr("FoamyOS x86 Bare-Metal Kernel v1.0\n");

        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_putstr("      /  \\       Arch: ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_putstr("i686 Protected Mode (Multiboot 1 Compliant)\n");

        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_putstr("     / /\\ \\      Display: ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_putstr("VGA Color Text Mode 80x25 @ 0xB8000\n");

        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_putstr("    / /  \\ \\     Input: ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_putstr("PS/2 Keyboard Driver (IRQ1 Enabled)\n");

        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_putstr("   /_/    \\_\\    Status: ");
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_putstr("Kernel Active & Healthy\n");

        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
    else if (strcmp(name, "color") == 0) {
        if (*arg == '\0') {
            vga_putstr("Usage: color <fg: 0..15> <bg: 0..15>\n");
        } else {
            int fg = 7, bg = 0;
            char* ptr = arg;
            fg = parse_num(ptr);
            while (*ptr && *ptr != ' ') ptr++;
            while (*ptr == ' ') ptr++;
            if (*ptr != '\0') bg = parse_num(ptr);

            vga_set_color((vga_color_t)(fg & 0xF), (vga_color_t)(bg & 0xF));
            vga_putstr("Color palette updated!\n");
        }
    }
    else if (strcmp(name, "peek") == 0) {
        if (*arg == '\0') {
            vga_putstr("Usage: peek <address in dec or hex 0x...>\n");
        } else {
            uint32_t addr = parse_num(arg);
            uint8_t val = *(volatile uint8_t*)addr;
            vga_putstr("Memory [");
            vga_puthex(addr);
            vga_putstr("] = ");
            vga_puthex(val);
            vga_putchar('\n');
        }
    }
    else if (strcmp(name, "poke") == 0) {
        if (*arg == '\0') {
            vga_putstr("Usage: poke <address> <val>\n");
        } else {
            char* ptr = arg;
            uint32_t addr = parse_num(ptr);
            while (*ptr && *ptr != ' ') ptr++;
            while (*ptr == ' ') ptr++;
            uint8_t val = (uint8_t)parse_num(ptr);

            *(volatile uint8_t*)addr = val;
            vga_putstr("Wrote ");
            vga_puthex(val);
            vga_putstr(" to ");
            vga_puthex(addr);
            vga_putchar('\n');
        }
    }
    else if (strcmp(name, "panic") == 0) {
        __asm__ __volatile__("int $0x0"); /* Division by zero exception test */
    }
    else if (strcmp(name, "reboot") == 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_putstr("Rebooting system...\n");
        uint8_t good = 0x02;
        while (good & 0x02) {
            good = inb(0x64);
        }
        outb(0x64, 0xFE);
        for (;;) { __asm__ __volatile__("hlt"); }
    }
    else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_putstr("Unknown command: ");
        vga_putstr(name);
        vga_putstr(". Type 'help' for commands.\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
}

void shell_init(void) {
    cmd_idx = 0;
    print_prompt();
}

void shell_run(void) {
    while (1) {
        char c = keyboard_getc();

        if (c == '\n') {
            vga_putchar('\n');
            cmd_buf[cmd_idx] = '\0';
            execute_command(cmd_buf);
            cmd_idx = 0;
            print_prompt();
        } else if (c == '\b') {
            if (cmd_idx > 0) {
                cmd_idx--;
                vga_putchar('\b');
            }
        } else if (c >= 32 && c <= 126) {
            if (cmd_idx < MAX_CMD_LEN - 1) {
                cmd_buf[cmd_idx++] = c;
                vga_putchar(c);
            }
        }
    }
}
