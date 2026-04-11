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
    (void)tty;
    (void)mode;
}

enum tty_mode tty_get_mode(const struct tty *tty) {
    (void)tty;
    return TTY_MODE_COOKED;
}

char tty_read_raw_blocking(struct tty *tty) {
    (void)tty;
    return 0;
}

int tty_read(struct tty *tty, char *buf, int len) {
    (void)tty;
    (void)buf;
    (void)len;
    return 0;
}

int tty_write(struct tty *tty, const char *buf, int len) {
    (void)tty;
    (void)buf;
    (void)len;
    return 0;
}
