#include <stdbool.h>
#include <stdint.h>

#include "mos-ports.h"
#include "mos-stdio.h"

#define VGA_MEMORY ((volatile uint8_t*)0x000b8000)
#define VGA_INDEX 0x03d4
#define VGA_DATA 0x03d5
#define VGA_ROW_WIDTH 80

static uint16_t character_ptr = 0;

static void move_cursor(uint16_t amt, bool backwards) {
    uint16_t cursor_pos;

    write_port_byte(VGA_INDEX, 0x0F);
    cursor_pos = read_port_byte(VGA_DATA);
    write_port_byte(VGA_INDEX, 0x0E);
    cursor_pos |= ((uint16_t)read_port_byte(VGA_DATA)) << 8;

    if (backwards) {
        cursor_pos -= amt;
    } else {
        cursor_pos += amt;
    }

    write_port_byte(VGA_INDEX, 0x0F);
    write_port_byte(VGA_DATA, (uint8_t)(cursor_pos & 0xFF));
    write_port_byte(VGA_INDEX, 0x0E);
    write_port_byte(VGA_DATA, (uint8_t)((cursor_pos >> 8) & 0xFF));
}

void print_char(uint8_t ch) {
    VGA_MEMORY[character_ptr] = ch;
    VGA_MEMORY[character_ptr + 1] = 0x0f;
    character_ptr += 2;
    move_cursor(1, false);
}

void printf(char* str) {
    int str_pointer = 0;

    while (str[str_pointer] != 0) {
        print_char(str[str_pointer]);
        str_pointer++;
    }
}

void print_line(char* str) {
    int str_pointer = 0;
    uint16_t count = 0;

    while (str[str_pointer] != 0) {
        print_char(str[str_pointer]);
        str_pointer++;
        count++;
    }

    character_ptr -= count * 2;
    character_ptr += VGA_ROW_WIDTH * 2;
    move_cursor(count, true);
    move_cursor(VGA_ROW_WIDTH, false);
}
