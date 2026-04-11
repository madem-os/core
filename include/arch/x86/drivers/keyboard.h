/*
 * x86 Keyboard Driver Interface
 *
 * This header declares the public entry point for the PS/2 keyboard driver
 * used by the current x86 kernel build.
 *
 * Responsibilities in this header:
 * - expose driver initialization so the kernel can register the IRQ handler
 * - mark the keyboard driver as an x86-specific hardware module
 * - keep keyboard-specific implementation details private to keyboard.c
 *
 * The driver currently owns simple scancode translation internally and writes
 * characters through the legacy console path. That behavior is transitional
 * and will be replaced as the newer input and TTY stack is integrated.
 */

#ifndef ARCH_X86_DRIVERS_KEYBOARD_H
#define ARCH_X86_DRIVERS_KEYBOARD_H

void keyboard_init(void);

#endif
