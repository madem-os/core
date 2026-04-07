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
    character_ptr += 2;
    move_cursor(1, false);
}

int hex_to_nibble(uint8_t code) {
    if (code < 10) {
        code += '0';
    }
    else {
        code += '0';
        code += 7;
    }
    return code;
}

void print_byte(uint8_t code) {
    uint8_t low;
    uint8_t high;
    high = code;
    low = code;
    high = high >> 4;
    high = hex_to_nibble(high);
    low = low & 0x0f;
    low = hex_to_nibble(low);
    print_char('0');
    print_char('x');
    print_char(high);
    print_char(low);
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

const char scancode_to_ascii[] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0
};

char get_ascii(uint8_t scancode) {
    if (scancode < sizeof(scancode_to_ascii)) {
      return scancode_to_ascii[scancode];
    }
    return 0;
}



uint8_t input(void) {
    while (true)
    {
        while (!(read_port_byte(0x64) & 1)) {}
        uint8_t ch = read_port_byte(0x60);
        return ch;
    }
}

static int shift_pressed = 0;
static int ctrl_pressed = 0;

void process_scancode(uint8_t scancode) {
    if (scancode == 0x2A) shift_pressed = 1;      // left shift make
    else if (scancode == 0xAA) shift_pressed = 0;  // left shift break
    else if (scancode == 0x36) shift_pressed = 1;  // right shift break
    else if (scancode == 0xB8) shift_pressed = 0;  // right shift break
    else if (scancode & 0x80) {} // Break code - not printable
    else {
        char ascii = get_ascii(scancode);
        if (shift_pressed && ascii >= 'a' && ascii <= 'z') {
            ascii -= 32;  // Convert to uppercase
        }
        print_char((uint8_t)ascii);
    }
}