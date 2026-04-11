/*
 * x86 PS/2 Keyboard Driver
 *
 * This file owns keyboard-specific behavior for the current x86 kernel,
 * including IRQ registration, scancode tracking, and translation of simple
 * key presses into characters written to the console.
 *
 * Responsibilities in this file:
 * - register the keyboard IRQ handler with the IRQ dispatcher
 * - unmask IRQ1 on the PIC during driver initialization
 * - read raw scancodes from the keyboard data port
 * - translate the current scancode set subset used by the project
 *
 * This file should not configure the IDT or PIC globally, and it should not
 * implement VGA rendering details. It consumes IRQ and console services
 * provided by those neighboring modules.
 *
 * The translation logic remains intentionally small and only supports the
 * subset of keys needed by the current demos and tests.
 */

#include <stdbool.h>
#include <stdint.h>

#include "arch/x86/drivers/keyboard.h"
#include "arch/x86/irq.h"
#include "arch/x86/pic.h"
#include "arch/x86/ports.h"

#define KEYBOARD_IRQ 1
#define KEYBOARD_DATA_PORT 0x60

kb_driver_output_writer_t kbwriter;

static void keyboard_irq_handler(void) {
    uint8_t scancode = read_port_byte(KEYBOARD_DATA_PORT);
    kbwriter(scancode);
}

void keyboard_init(kb_driver_output_writer_t writer) {
    kbwriter = writer;
    irq_install_handler(KEYBOARD_IRQ, keyboard_irq_handler);
    pic_clear_mask(KEYBOARD_IRQ);
}
