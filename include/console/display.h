/*
 * Display Interface
 *
 * This header declares the low-level display abstraction used by the new text
 * console module. A display backend writes character cells and synchronizes the
 * visible cursor on some concrete output device.
 *
 * The first slice keeps this generic and testable. A later VGA-specific
 * backend can implement the callbacks using text-buffer writes and cursor-port
 * updates without changing the terminal-facing console logic.
 */

#ifndef CONSOLE_DISPLAY_H
#define CONSOLE_DISPLAY_H

#include <stdint.h>

struct display;

typedef void (*display_write_cell_t)(
    struct display *display,
    uint16_t row,
    uint16_t column,
    uint8_t ch,
    uint8_t color
);

typedef void (*display_set_cursor_t)(
    struct display *display,
    uint16_t row,
    uint16_t column
);

struct display {
    uint16_t *buffer;
    uint16_t width;
    uint16_t height;
    void *ctx;
    display_write_cell_t write_cell;
    display_set_cursor_t set_cursor;
};

void display_init(
    struct display *display,
    uint16_t *buffer,
    uint16_t width,
    uint16_t height,
    void *ctx,
    display_write_cell_t write_cell,
    display_set_cursor_t set_cursor
);

#endif
