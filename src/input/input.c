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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "input/input.h"
#include "input/input_ring.h"

#define INPUT_SCANCODE_EXTENDED_PREFIX 0xE0u
#define INPUT_SCANCODE_RELEASE_MASK 0x80u
#define INPUT_LEFT_SHIFT_PRESS 0x2Au
#define INPUT_RIGHT_SHIFT_PRESS 0x36u
#define INPUT_LEFT_SHIFT_RELEASE 0xAAu
#define INPUT_RIGHT_SHIFT_RELEASE 0xB6u

struct input_state {
    struct input_ring ring;
    bool shift_pressed;
    bool extended_prefix_seen;
};

static struct input_state input_state;

static const uint8_t input_scancode_to_ascii[] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0
};

static uint8_t input_apply_shift(uint8_t value) {
    switch (value) {
        case '1':
            return '!';
        case '2':
            return '@';
        case '3':
            return '#';
        case '4':
            return '$';
        case '5':
            return '%';
        case '6':
            return '^';
        case '7':
            return '&';
        case '8':
            return '*';
        case '9':
            return '(';
        case '0':
            return ')';
        case '-':
            return '_';
        case '=':
            return '+';
        case '[':
            return '{';
        case ']':
            return '}';
        case ';':
            return ':';
        case '\'':
            return '"';
        case '`':
            return '~';
        case '\\':
            return '|';
        case ',':
            return '<';
        case '.':
            return '>';
        case '/':
            return '?';
        default:
            if (value >= 'a' && value <= 'z') {
                return (uint8_t)(value - ('a' - 'A'));
            }
            return value;
    }
}

static void input_enqueue_byte(uint8_t value) {    
    (void)input_ring_push(&input_state.ring, value);
}

static void input_handle_extended_scancode(uint8_t scancode) {
    switch (scancode) {
        case 0x48:
            input_enqueue_byte(0x1B);
            input_enqueue_byte('[');
            input_enqueue_byte('A');
            break;
        case 0x50:
            input_enqueue_byte(0x1B);
            input_enqueue_byte('[');
            input_enqueue_byte('B');
            break;
        case 0x4B:
            input_enqueue_byte(0x1B);
            input_enqueue_byte('[');
            input_enqueue_byte('D');
            break;
        case 0x4D:
            input_enqueue_byte(0x1B);
            input_enqueue_byte('[');
            input_enqueue_byte('C');
            break;
        default:
            break;
    }
}

void input_init(void) {
    input_ring_init(&input_state.ring);
    input_state.shift_pressed = false;
    input_state.extended_prefix_seen = false;
}

void input_handle_scancode(uint8_t scancode) {
    uint8_t value;

    if (scancode == INPUT_SCANCODE_EXTENDED_PREFIX) {
        input_state.extended_prefix_seen = true;
        return;
    }

    if (input_state.extended_prefix_seen) {
        input_state.extended_prefix_seen = false;
        if ((scancode & INPUT_SCANCODE_RELEASE_MASK) != 0) {
            return;
        }
        input_handle_extended_scancode(scancode);
        return;
    }

    if (scancode == INPUT_LEFT_SHIFT_PRESS || scancode == INPUT_RIGHT_SHIFT_PRESS) {
        input_state.shift_pressed = true;
        return;
    }

    if (scancode == INPUT_LEFT_SHIFT_RELEASE || scancode == INPUT_RIGHT_SHIFT_RELEASE) {
        input_state.shift_pressed = false;
        return;
    }

    if ((scancode & INPUT_SCANCODE_RELEASE_MASK) != 0) {
        return;
    }

    if (scancode >= sizeof(input_scancode_to_ascii)) {
        return;
    }

    value = input_scancode_to_ascii[scancode];
    if (value == 0) {
        return;
    }

    if (input_state.shift_pressed) {
        value = input_apply_shift(value);
    }

    input_enqueue_byte(value);
}

bool input_has_byte(void) {
    return !input_ring_is_empty(&input_state.ring);
}

bool input_read_byte(uint8_t *value) {
    return input_ring_pop(&input_state.ring, value);
}

uint8_t input_read_byte_blocking(void) {
    uint8_t value = 0;

    while (!input_read_byte(&value)) {
    }

    return value;
}
