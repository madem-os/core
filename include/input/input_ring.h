/*
 * Input Ring Buffer Interface
 *
 * This header declares the fixed-size byte queue used by the input system to
 * store translated terminal input bytes before higher layers consume them.
 *
 * The ring is intentionally generic within the input subsystem. It has no
 * dependency on port I/O, IRQ setup, or terminal policy, which keeps it easy
 * to reuse and unit-test in hosted builds.
 */

#ifndef INPUT_INPUT_RING_H
#define INPUT_INPUT_RING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INPUT_RING_CAPACITY 128u

struct input_ring {
    uint8_t data[INPUT_RING_CAPACITY];
    volatile size_t head;
    volatile size_t tail;
};

void input_ring_init(struct input_ring *ring);
bool input_ring_is_empty(const struct input_ring *ring);
bool input_ring_is_full(const struct input_ring *ring);
bool input_ring_push(struct input_ring *ring, uint8_t value);
bool input_ring_pop(struct input_ring *ring, uint8_t *value);

#endif
