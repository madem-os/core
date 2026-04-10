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
    ring->head = 0;
    ring->tail = 0;
}

bool keyboard_ring_is_empty(const struct keyboard_ring *ring) {
    return ring->head == ring->tail;
}

bool keyboard_ring_is_full(const struct keyboard_ring *ring) {
    int normalized_tail = (int)ring->tail - (int)ring->head;
    if (normalized_tail < 0) {
        normalized_tail += (int)KEYBOARD_RING_CAPACITY;
    }
    return normalized_tail == (int)KEYBOARD_RING_CAPACITY - 1;
}

bool keyboard_ring_push(struct keyboard_ring *ring, uint8_t value) {
    if (keyboard_ring_is_full(ring)) {
        return false;
    }
    ring->data[ring->tail] = value;
    ring->tail = (ring->tail + 1) % KEYBOARD_RING_CAPACITY;
    return true;
}

bool keyboard_ring_pop(struct keyboard_ring *ring, uint8_t *value) {
    if (keyboard_ring_is_empty(ring)) {
        return false;
    }
    *value = ring->data[ring->head];
    ring->head = (ring->head + 1) % KEYBOARD_RING_CAPACITY;
    return true;
}
