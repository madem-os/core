/*
 * VGA Text Console
 *
 * This file owns the current text-mode console used by the kernel to show
 * boot messages and simple character output on the VGA text buffer.
 *
 * Responsibilities in this file:
 * - write characters and strings into VGA text memory
 * - keep track of the software cursor used by this project
 * - synchronize that position with the hardware text cursor registers
 * - provide a tiny formatting surface appropriate for early boot output
 *
 * This file should not implement keyboard IRQ registration, scancode
 * translation, PIC setup, or generic stdio semantics. It is a console module,
 * not a full terminal or libc layer.
 *
 * The implementation assumes VGA text mode is already active and that the
 * kernel uses direct writes to 0xB8000 during early bring-up.
 */

#include <stdbool.h>
#include <stdint.h>

#include "arch/x86/ports.h"
#include "console/console.h"

#define VGA_MEMORY ((volatile uint8_t*)0x000B8000)
#define VGA_INDEX 0x03D4
#define VGA_DATA 0x03D5
#define VGA_ROW_WIDTH 80

static uint16_t character_ptr;

static int console_hex_to_nibble(uint8_t code) {
    if (code < 10) {
        code += '0';
    } else {
        code += '0';
        code += 7;
    }

    return code;
}

static void console_move_cursor(uint16_t amount, bool backwards) {
    uint16_t cursor_pos;

    write_port_byte(VGA_INDEX, 0x0F);
    cursor_pos = read_port_byte(VGA_DATA);
    write_port_byte(VGA_INDEX, 0x0E);
    cursor_pos |= ((uint16_t)read_port_byte(VGA_DATA)) << 8;

    if (backwards) {
        cursor_pos -= amount;
    } else {
        cursor_pos += amount;
    }

    write_port_byte(VGA_INDEX, 0x0F);
    write_port_byte(VGA_DATA, (uint8_t)(cursor_pos & 0xFF));
    write_port_byte(VGA_INDEX, 0x0E);
    write_port_byte(VGA_DATA, (uint8_t)((cursor_pos >> 8) & 0xFF));
}

static void console_move_cursor_to(uint8_t row, uint8_t column) {
    uint16_t cursor_pos = (uint16_t)(row * VGA_ROW_WIDTH + column);

    write_port_byte(VGA_INDEX, 0x0F);
    write_port_byte(VGA_DATA, (uint8_t)(cursor_pos & 0xFF));
    write_port_byte(VGA_INDEX, 0x0E);
    write_port_byte(VGA_DATA, (uint8_t)((cursor_pos >> 8) & 0xFF));
}

int console_cursor_position(void) {
    uint16_t cursor_pos;

    write_port_byte(VGA_INDEX, 0x0F);
    cursor_pos = read_port_byte(VGA_DATA);
    write_port_byte(VGA_INDEX, 0x0E);
    cursor_pos |= ((uint16_t)read_port_byte(VGA_DATA)) << 8;

    return (int)cursor_pos;
}

void console_write_char(uint8_t ch) {
    VGA_MEMORY[character_ptr] = ch;
    character_ptr += 2;
    console_move_cursor(1, false);
}

void console_write_hex_byte(uint8_t code) {
    uint8_t low = code & 0x0F;
    uint8_t high = (uint8_t)(code >> 4);

    console_write_char('0');
    console_write_char('x');
    console_write_char((uint8_t)console_hex_to_nibble(high));
    console_write_char((uint8_t)console_hex_to_nibble(low));
}

void console_write(char* str) {
    int str_pointer = 0;

    while (str[str_pointer] != 0) {
        console_write_char((uint8_t)str[str_pointer]);
        str_pointer++;
    }
}

void console_write_char_at(uint8_t ch, uint8_t row, uint8_t column) {
    uint16_t cursor = (uint16_t)(row * 160 + column * 2);

    console_move_cursor_to(row, (uint8_t)(column + 1));
    VGA_MEMORY[cursor] = ch;
}

void console_write_at(char* str, uint8_t row, uint8_t column) {
    int str_pointer = 0;

    while (str[str_pointer] != 0) {
        console_write_char_at((uint8_t)str[str_pointer], row, column);
        str_pointer++;
        column++;
        if (column >= VGA_ROW_WIDTH) {
            row++;
            column = 0;
        }
    }
}

void console_write_line(char* str) {
    int str_pointer = 0;
    uint16_t count = 0;

    while (str[str_pointer] != 0) {
        console_write_char((uint8_t)str[str_pointer]);
        str_pointer++;
        count++;
    }

    character_ptr -= count * 2;
    character_ptr += VGA_ROW_WIDTH * 2;
    console_move_cursor(count, true);
    console_move_cursor(VGA_ROW_WIDTH, false);
}
