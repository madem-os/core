/*
 * Text Console Interface
 *
 * This header declares the new terminal-facing console backend that sits
 * between TTY output and a display backend. It interprets byte streams such as
 * printable characters, newline, and backspace, then renders them through the
 * injected display abstraction.
 *
 * This is intentionally a new layer and does not reuse the legacy VGA-coupled
 * console module.
 */

#ifndef CONSOLE_TEXT_CONSOLE_H
#define CONSOLE_TEXT_CONSOLE_H

#include <stdint.h>

#include "console/display.h"

struct text_console {
    struct display *display;
    uint16_t row;
    uint16_t column;
    uint8_t color;
    uint8_t wrap_pending;
};

void text_console_init(struct text_console *console, struct display *display);
int text_console_write(struct text_console *console, const char *buf, int len);
void text_console_clear(struct text_console *console);
int text_console_tty_write(void *ctx, const char *buf, int len);

#endif
