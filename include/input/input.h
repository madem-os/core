/*
 * Input System Interface
 *
 * This header declares the byte-oriented input layer that sits between the
 * raw keyboard driver and higher terminal-facing code. It accepts scancodes,
 * translates them into terminal input bytes, and exposes buffered reads.
 *
 * The first TDD slice only defines the API. The implementation starts as a
 * stub so hosted tests can drive the desired behavior before integration work
 * begins.
 */

#ifndef INPUT_INPUT_H
#define INPUT_INPUT_H

#include <stdbool.h>
#include <stdint.h>

void input_init(void);
void input_handle_scancode(uint8_t scancode);
bool input_has_byte(void);
bool input_read_byte(uint8_t *value);
uint8_t input_read_byte_blocking(void);

#endif
