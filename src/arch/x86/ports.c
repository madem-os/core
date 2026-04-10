/*
 * x86 Port I/O Primitives
 *
 * This file owns the raw inb/outb/outw style helpers used by x86-specific
 * subsystems to talk to hardware through legacy I/O ports.
 *
 * Responsibilities in this file:
 * - provide minimal wrappers around x86 port instructions
 * - expose a tiny delay helper for controllers that need an I/O wait
 * - stay policy-free so higher layers can build on stable primitives
 *
 * This file should not contain PIC initialization, keyboard handling,
 * console logic, or interrupt routing decisions. It is a leaf module used by
 * those higher-level components.
 *
 * The helpers here assume the kernel already runs in 32-bit x86 mode and
 * that callers understand the port semantics of the hardware they access.
 */

#include <stdint.h>

#include "arch/x86/ports.h"

void write_port_byte(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t read_port_byte(uint16_t port) {
    uint8_t value;

    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void write_port_word(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}
