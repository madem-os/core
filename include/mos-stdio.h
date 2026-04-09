#ifndef MOS_STDIO_H
#define MOS_STDIO_H

#include <stdint.h>
#include <stdbool.h>

void print_char(uint8_t ch);
void print_line(char* str);
void printf(char* str);
uint8_t input(void);
char get_ascii(uint8_t scancode);
void process_scancode(uint8_t scancode);
void print_byte(uint8_t code);
void move_cursor(uint16_t amt, bool backwards);
void text_mode();
void hex_debug_mode();
void modes_init();
void print(char* str, uint8_t row, uint8_t coulmn);
int pull_cursor();

#endif
