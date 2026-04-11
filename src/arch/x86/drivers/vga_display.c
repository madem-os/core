/*
 * x86 VGA Display Backend
 *
 * This file adapts the generic display interface to the legacy x86 VGA
 * text-mode device. It owns the concrete text-buffer address and the hardware
 * cursor port protocol, while exposing the backend through `struct display`.
 *
 * Responsibilities in this file:
 * - write character/color cells to VGA text memory at 0xB8000
 * - update the hardware cursor through ports 0x3D4 and 0x3D5
 * - initialize a generic display object with VGA-specific geometry and hooks
 *
 * This file should remain a backend only. It should not interpret terminal
 * bytes, implement scrolling policy, or own any TTY semantics. Those belong in
 * the text console layer above it.
 *
 * The optional `ctx` pointer is carried through the generic display object for
 * future backend state if needed, but the first implementation relies only on
 * the fixed VGA text-mode buffer and cursor registers.
 */

#include <stdint.h>

#include "arch/x86/drivers/vga_display.h"
#include "arch/x86/ports.h"
#include "console/display.h"

#define VGA_TEXT_BUFFER ((uint16_t *)0x000B8000u)
#define VGA_WIDTH 80u
#define VGA_HEIGHT 25u
#define VGA_INDEX_PORT 0x03D4u
#define VGA_DATA_PORT 0x03D5u
#define VGA_CURSOR_LOW 0x0Fu
#define VGA_CURSOR_HIGH 0x0Eu

void vga_display_write_cell(
    struct display *display,
    uint16_t row,
    uint16_t column,
    uint8_t ch,
    uint8_t color
) {
    uint16_t offset;
    offset = (uint16_t)(row * display->width + column);
    display->buffer[offset] = (uint16_t)ch | ((uint16_t)color << 8);
}

void vga_display_set_cursor(
    struct display *display,
    uint16_t row,
    uint16_t column
) {
    uint16_t position;

    position = (uint16_t)(row * display->width + column);

    write_port_byte(VGA_INDEX_PORT, VGA_CURSOR_LOW);
    write_port_byte(VGA_DATA_PORT, (uint8_t)(position & 0x00FFu));
    write_port_byte(VGA_INDEX_PORT, VGA_CURSOR_HIGH);
    write_port_byte(VGA_DATA_PORT, (uint8_t)((position >> 8) & 0x00FFu));
}

void init_vga_display(struct display *display, void *ctx) {
    display_init(
        display,
        VGA_TEXT_BUFFER,
        VGA_WIDTH,
        VGA_HEIGHT,
        ctx,
        vga_display_write_cell,
        vga_display_set_cursor
    );
}
