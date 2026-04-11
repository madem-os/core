/*
 * Input System
 *
 * This module will translate keyboard scancodes into terminal input bytes and
 * buffer them for higher layers such as TTY. The full implementation will own
 * translation state, buffering policy, and blocking/non-blocking reads.
 *
 * The current TDD slice intentionally leaves the behavior stubbed so the
 * hosted unit tests can define the target contract first.
 */

#include "input/input.h"

void input_init(void) {
}

void input_handle_scancode(uint8_t scancode) {
    (void)scancode;
}

bool input_has_byte(void) {
    return false;
}

bool input_read_byte(uint8_t *value) {
    (void)value;
    return false;
}

uint8_t input_read_byte_blocking(void) {
    return 0;
}
