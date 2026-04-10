/*
 * Keyboard Driver Interface
 *
 * This header declares the public entry point for the PS/2 keyboard driver
 * used by the current kernel build.
 *
 * Responsibilities in this header:
 * - expose driver initialization so the kernel can register the IRQ handler
 * - keep keyboard-specific implementation details private to keyboard.c
 *
 * The driver currently owns simple scancode translation internally and writes
 * characters through the console module.
 */

#ifndef DRIVERS_KEYBOARD_H
#define DRIVERS_KEYBOARD_H

void keyboard_init(void);

#endif
