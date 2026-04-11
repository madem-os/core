/*
 * Display Backend
 *
 * This file stores the low-level display object used by the new text console
 * module. Device-specific behavior is provided through callbacks, which keeps
 * the first implementation easy to test in hosted builds.
 *
 * The first TDD slice only initializes the display object. A future VGA
 * backend will supply concrete cell-writing and cursor-moving operations.
 */

#include "console/display.h"

void display_init(
    struct display *display,
    uint16_t *buffer,
    uint16_t width,
    uint16_t height,
    void *ctx,
    display_write_cell_t write_cell,
    display_set_cursor_t set_cursor
) {
    display->buffer = buffer;
    display->width = width;
    display->height = height;
    display->ctx = ctx;
    display->write_cell = write_cell;
    display->set_cursor = set_cursor;
}
