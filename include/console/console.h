/*
 * VGA Text Console Interface
 *
 * This header declares the small console API used by the kernel and drivers
 * to write human-readable status text to the VGA text buffer.
 *
 * Responsibilities in this header:
 * - expose character and string output helpers
 * - expose explicit positioned writes for simple screen layout needs
 * - expose small debug-oriented helpers such as hex byte output
 *
 * This is not a generic stdio layer. Input handling, keyboard translation,
 * and interrupt plumbing belong to other modules.
 */

#ifndef CONSOLE_CONSOLE_H
#define CONSOLE_CONSOLE_H

#include <stdint.h>

void console_write_char(uint8_t ch);
void console_write(char* str);
void console_write_line(char* str);
void console_write_hex_byte(uint8_t code);
void console_write_char_at(uint8_t ch, uint8_t row, uint8_t column);
void console_write_at(char* str, uint8_t row, uint8_t column);
int console_cursor_position(void);

#endif
