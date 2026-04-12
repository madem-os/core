/*
 * Kernel Panic Output
 *
 * This file implements the fatal-error reporting path used by early exception
 * handling. It prefers the active text console once initialized, but falls
 * back to direct VGA text memory writes so panic messages remain visible even
 * if higher layers are unavailable.
 *
 * Responsibilities in this file:
 * - remember the active text console for panic output
 * - provide minimal string and hex writers independent of fd/TTY dispatch
 * - halt the machine after reporting an unrecoverable panic
 *
 * This module intentionally stays simple. It is not a structured logger and
 * does not attempt recovery.
 */

#include <stddef.h>
#include <stdint.h>

#include "arch/x86/lowlevel.h"
#include "console/text_console.h"
#include "kernel/panic.h"

#define PANIC_VGA_BUFFER ((volatile uint16_t *)0xC00B8000u)
#define PANIC_VGA_WIDTH 80u
#define PANIC_VGA_HEIGHT 25u
#define PANIC_VGA_COLOR 0x4Fu

static struct text_console *panic_console;
static uint16_t panic_fallback_row;
static uint16_t panic_fallback_column;

static void panic_write_fallback_char(uint8_t ch) {
    if (ch == (uint8_t)'\n') {
        panic_fallback_column = 0;
        if (panic_fallback_row + 1u < PANIC_VGA_HEIGHT) {
            panic_fallback_row++;
        }
        return;
    }

    PANIC_VGA_BUFFER[
        panic_fallback_row * PANIC_VGA_WIDTH + panic_fallback_column
    ] = (uint16_t)ch | ((uint16_t)PANIC_VGA_COLOR << 8);

    panic_fallback_column++;
    if (panic_fallback_column >= PANIC_VGA_WIDTH) {
        panic_fallback_column = 0;
        if (panic_fallback_row + 1u < PANIC_VGA_HEIGHT) {
            panic_fallback_row++;
        }
    }
}

void panic_set_console(struct text_console *console) {
    panic_console = console;
}

void panic_write(const char *text) {
    size_t length;

    if (text == NULL) {
        return;
    }

    if (panic_console != NULL) {
        for (length = 0; text[length] != '\0'; length++) {
        }
        (void)text_console_write(panic_console, text, (int)length);
        return;
    }

    for (length = 0; text[length] != '\0'; length++) {
        panic_write_fallback_char((uint8_t)text[length]);
    }
}

void panic_write_hex32(uint32_t value) {
    static const char hex_digits[] = "0123456789ABCDEF";
    char hex_buffer[11];
    int index;
    int shift;

    hex_buffer[0] = '0';
    hex_buffer[1] = 'x';
    for (index = 0, shift = 28; index < 8; index++, shift -= 4) {
        hex_buffer[index + 2] = hex_digits[(value >> shift) & 0x0Fu];
    }
    hex_buffer[10] = '\0';

    panic_write(hex_buffer);
}

void panic_die(const char *message) {
    panic_write("panic: ");
    panic_write(message);
    panic_write("\n");
    x86_halt_forever();
}
