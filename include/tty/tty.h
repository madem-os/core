/*
 * TTY Interface
 *
 * This header declares the first standalone terminal layer that sits above the
 * input system's translated byte stream. The TTY owns terminal semantics such
 * as raw versus cooked reads and optional echo through abstract display hooks.
 *
 * The first implementation slice is test-driven and starts as a stub. It is
 * intentionally isolated from keyboard hardware, IRQ setup, and file-descriptor
 * routing so its behavior can be validated with hosted unit tests first.
 */

#ifndef TTY_TTY_H
#define TTY_TTY_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t (*tty_input_reader_t)(void);
typedef int (*tty_output_writer_t)(void *ctx, const char *buf, int len);

enum tty_mode {
    TTY_MODE_COOKED = 0,
    TTY_MODE_RAW = 1
};

struct tty {
    enum tty_mode mode;
    bool echo_enabled;
    tty_input_reader_t read_input_byte_blocking;
    tty_output_writer_t write_output;
    void *output_ctx;
};

void tty_init(
    struct tty *tty,
    tty_input_reader_t reader,
    tty_output_writer_t writer,
    void *output_ctx
);
void tty_set_mode(struct tty *tty, enum tty_mode mode);
enum tty_mode tty_get_mode(const struct tty *tty);
char tty_read_raw_blocking(struct tty *tty);
int tty_read(struct tty *tty, char *buf, int len);
int tty_write(struct tty *tty, const char *buf, int len);

#endif
