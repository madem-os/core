/*
 * TTY Module
 *
 * This file will implement the first terminal abstraction used by the kernel
 * input path. It consumes translated input bytes from an injected blocking
 * reader and will later provide raw and cooked terminal read behavior.
 *
 * The first TDD slice intentionally leaves the behavior stubbed so the hosted
 * tests can define the target contract before kernel integration begins.
 */

#include <stddef.h>
#include <stdint.h>

#include "tty/tty.h"

void tty_init(
    struct tty *tty,
    tty_input_reader_t reader,
    tty_output_writer_t writer,
    void *output_ctx
) {
    tty->mode = TTY_MODE_COOKED;
    tty->echo_enabled = true;
    tty->read_input_byte_blocking = reader;
    tty->write_output = writer;
    tty->output_ctx = output_ctx;
}

void tty_set_mode(struct tty *tty, enum tty_mode mode) {
    tty->mode = mode;
}

enum tty_mode tty_get_mode(const struct tty *tty) {
    return tty->mode;
}

char tty_read_raw_blocking(struct tty *tty) {
    return (char)tty->read_input_byte_blocking();
}

static int tty_echo_bytes(struct tty *tty, const char *buf, int len) {
    if (!tty->echo_enabled || tty->write_output == 0) {
        return len;
    }

    return tty_write(tty, buf, len);
}

int tty_read(struct tty *tty, char *buf, int len) {
    int count;
    uint8_t value;
    static const char tty_backspace_erase[] = "\b \b";

    if (len <= 0) {
        return 0;
    }

    if (tty->mode == TTY_MODE_RAW) {
        buf[0] = (char)tty->read_input_byte_blocking();
        return 1;
    }

    count = 0;
    for (;;) {
        value = tty->read_input_byte_blocking();

        if (value == '\b') {
            if (count > 0) {
                count--;
                (void)tty_echo_bytes(tty, tty_backspace_erase, 3);
            }
            continue;
        }

        if (value == '\n') {
            if (count < len) {
                buf[count++] = '\n';
            }
            (void)tty_echo_bytes(tty, "\n", 1);
            return count;
        }

        if (count < len) {
            buf[count++] = (char)value;
            (void)tty_echo_bytes(tty, (const char *)&buf[count - 1], 1);
        }
    }
}

int tty_write(struct tty *tty, const char *buf, int len) {
    if (tty->write_output == 0) {
        return len;
    }

    return tty->write_output(tty->output_ctx, buf, len);
}
