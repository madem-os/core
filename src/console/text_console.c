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

void text_console_init(struct text_console *console, struct display *display) {
    console->display = display;
    console->row = 0;
    console->column = 0;
    console->color = 0x07u;
}

int text_console_write(struct text_console *console, const char *buf, int len) {
    (void)console;
    (void)buf;
    (void)len;
    return 0;
}

void text_console_clear(struct text_console *console) {
    (void)console;
}

int text_console_tty_write(void *ctx, const char *buf, int len) {
    return text_console_write((struct text_console *)ctx, buf, len);
}
