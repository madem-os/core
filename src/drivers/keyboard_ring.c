/*
 * Keyboard Ring Buffer
 *
 * This module will provide the low-level byte queue used by the keyboard
 * driver. It is intentionally kept free of port I/O and IRQ registration so
 * it can be unit-tested as normal hosted C code.
 *
 * The implementation is intentionally stubbed for the first TDD slice.
 */

#include "drivers/keyboard_ring.h"

void keyboard_ring_init(struct keyboard_ring *ring) {
    (void)ring;
}

bool keyboard_ring_is_empty(const struct keyboard_ring *ring) {
    (void)ring;
    return false;
}

bool keyboard_ring_is_full(const struct keyboard_ring *ring) {
    (void)ring;
    return false;
}

bool keyboard_ring_push(struct keyboard_ring *ring, uint8_t value) {
    (void)ring;
    (void)value;
    return false;
}

bool keyboard_ring_pop(struct keyboard_ring *ring, uint8_t *value) {
    (void)ring;
    (void)value;
    return false;
}
