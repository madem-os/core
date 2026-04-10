/*
 * Keyboard Ring Buffer Interface
 *
 * This header declares the fixed-size single-producer / single-consumer ring
 * buffer that will back low-level keyboard input in the kernel.
 *
 * The first implementation slice uses byte-sized entries so IRQ-side keyboard
 * code can enqueue raw scancodes without depending on console or TTY logic.
 */

#ifndef DRIVERS_KEYBOARD_RING_H
#define DRIVERS_KEYBOARD_RING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KEYBOARD_RING_CAPACITY 128u

struct keyboard_ring {
    uint8_t data[KEYBOARD_RING_CAPACITY];
    volatile size_t head;
    volatile size_t tail;
};

void keyboard_ring_init(struct keyboard_ring *ring);
bool keyboard_ring_is_empty(const struct keyboard_ring *ring);
bool keyboard_ring_is_full(const struct keyboard_ring *ring);
bool keyboard_ring_push(struct keyboard_ring *ring, uint8_t value);
bool keyboard_ring_pop(struct keyboard_ring *ring, uint8_t *value);

#endif
