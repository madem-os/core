/*
 * Input Ring Buffer
 *
 * This module implements the low-level byte queue used by the input system.
 * It stores translated terminal input bytes after scancode processing and
 * before TTY or other consumers read them.
 *
 * The implementation is kept independent from keyboard hardware and console
 * code so it can be validated with normal hosted unit tests.
 */

#include "input/input_ring.h"

void input_ring_init(struct input_ring *ring) {
    ring->head = 0;
    ring->tail = 0;
}

bool input_ring_is_empty(const struct input_ring *ring) {
    return ring->head == ring->tail;
}

bool input_ring_is_full(const struct input_ring *ring) {
    return (ring->tail - ring->head) == INPUT_RING_CAPACITY;
}

bool input_ring_push(struct input_ring *ring, uint8_t value) {
    size_t slot;

    if (input_ring_is_full(ring)) {
        return false;
    }

    slot = ring->tail % INPUT_RING_CAPACITY;
    ring->data[slot] = value;
    ring->tail++;
    return true;
}

bool input_ring_pop(struct input_ring *ring, uint8_t *value) {
    size_t slot;

    if (input_ring_is_empty(ring)) {
        return false;
    }

    slot = ring->head % INPUT_RING_CAPACITY;
    *value = ring->data[slot];
    ring->head++;
    return true;
}
