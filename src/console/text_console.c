/*
 * Text Console
 *
 * This file will implement the new terminal-facing console backend used behind
 * TTY output. It owns cursor semantics, newline/backspace handling, and
 * scrolling, while delegating actual device operations to the display backend.
 *
 * The first TDD slice intentionally leaves the behavior stubbed so hosted unit
 * tests can define the target contract before any VGA-specific backend exists.
 */

#include "console/text_console.h"

static void text_console_set_cursor(struct text_console *console) {
    uint16_t column;

    column = console->column;
    if (console->display->width != 0u && column >= console->display->width) {
        column = (uint16_t)(console->display->width - 1u);
    }

    if (console->display->set_cursor != 0) {
        console->display->set_cursor(
            console->display,
            console->row,
            column
        );
    }
}

static void text_console_write_cell(
    struct text_console *console,
    uint16_t row,
    uint16_t column,
    uint8_t ch
) {
    if (console->display->write_cell != 0) {
        console->display->write_cell(
            console->display,
            row,
            column,
            ch,
            console->color
        );
    }

    if (console->display->buffer != 0) {
        uint16_t offset;

        offset = (uint16_t)(row * console->display->width + column);
        console->display->buffer[offset] =
            (uint16_t)ch | ((uint16_t)console->color << 8);
    }
}

static void text_console_clear_row(struct text_console *console, uint16_t row) {
    uint16_t column;

    for (column = 0; column < console->display->width; column++) {
        text_console_write_cell(console, row, column, (uint8_t)' ');
    }
}

static void text_console_scroll(struct text_console *console) {
    uint16_t row;
    uint16_t column;

    for (row = 1; row < console->display->height; row++) {
        for (column = 0; column < console->display->width; column++) {
            uint16_t offset;
            uint16_t cell;
            uint8_t ch;
            uint8_t color;

            offset = (uint16_t)(row * console->display->width + column);
            cell = console->display->buffer[offset];
            ch = (uint8_t)(cell & 0x00FFu);
            color = (uint8_t)((cell >> 8) & 0x00FFu);

            if (console->display->write_cell != 0) {
                console->display->write_cell(
                    console->display,
                    (uint16_t)(row - 1u),
                    column,
                    ch,
                    color
                );
            }

            if (console->display->buffer != 0) {
                uint16_t target_offset;

                target_offset =
                    (uint16_t)((row - 1u) * console->display->width + column);
                console->display->buffer[target_offset] = cell;
            }
        }
    }

    text_console_clear_row(console, (uint16_t)(console->display->height - 1u));
}

static void text_console_advance_line(struct text_console *console) {
    console->column = 0;
    console->wrap_pending = 0u;

    if (console->row + 1u < console->display->height) {
        console->row++;
        return;
    }

    text_console_scroll(console);
}

void text_console_init(struct text_console *console, struct display *display) {
    console->display = display;
    console->row = 0;
    console->column = 0;
    console->color = 0x07u;
    console->wrap_pending = 0u;
    text_console_set_cursor(console);
}

int text_console_write(struct text_console *console, const char *buf, int len) {
    int index;

    if (buf == 0 || len <= 0) {
        return 0;
    }

    for (index = 0; index < len; index++) {
        uint8_t ch;

        ch = (uint8_t)buf[index];

        if (console->wrap_pending != 0u) {
            if (ch == (uint8_t)'\b') {
                console->wrap_pending = 0u;
                console->column = (uint16_t)(console->display->width - 1u);
            } else if (ch == (uint8_t)'\n') {
                text_console_advance_line(console);
                continue;
            } else {
                text_console_advance_line(console);
            }
        }

        if (ch == (uint8_t)'\n') {
            text_console_advance_line(console);
            continue;
        }

        if (ch == (uint8_t)'\b') {
            if (console->row != 0u || console->column != 0u) {
                if (console->column == 0u) {
                    console->row--;
                    console->column = (uint16_t)(console->display->width - 1u);
                } else {
                    console->column--;
                }

                console->wrap_pending = 0u;
                text_console_write_cell(
                    console,
                    console->row,
                    console->column,
                    (uint8_t)' '
                );
            }
            continue;
        }

        text_console_write_cell(console, console->row, console->column, ch);
        if (console->column + 1u >= console->display->width) {
            console->column = console->display->width;
            console->wrap_pending = 1u;
        } else {
            console->column++;
        }
    }

    text_console_set_cursor(console);
    return len;
}

void text_console_clear(struct text_console *console) {
    uint16_t row;

    for (row = 0; row < console->display->height; row++) {
        text_console_clear_row(console, row);
    }

    console->row = 0;
    console->column = 0;
    console->wrap_pending = 0u;
    text_console_set_cursor(console);
}

int text_console_tty_write(void *ctx, const char *buf, int len) {
    return text_console_write((struct text_console *)ctx, buf, len);
}
