/*
 * PS/2 Keyboard Driver
 *
 * This file owns keyboard-specific behavior for the current kernel, including
 * IRQ registration, scancode tracking, and translation of simple key presses
 * into characters written to the console.
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

#include "arch/x86/irq.h"
#include "arch/x86/pic.h"
#include "arch/x86/ports.h"
#include "console/console.h"
#include "drivers/keyboard.h"

#define KEYBOARD_IRQ 1
#define KEYBOARD_DATA_PORT 0x60

static const char scancode_to_ascii[] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0
};

static bool shift_pressed;

static char keyboard_translate_scancode(uint8_t scancode) {
    char ascii;

    if (scancode >= sizeof(scancode_to_ascii)) {
        return 0;
    }

    ascii = scancode_to_ascii[scancode];

    if (shift_pressed && ascii >= 'a' && ascii <= 'z') {
        ascii = (char)(ascii - ('a' - 'A'));
    }

    return ascii;
}

static void keyboard_process_scancode(uint8_t scancode) {
    char ascii;

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return;
    }

    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = false;
        return;
    }

    if ((scancode & 0x80u) != 0) {
        return;
    }

    ascii = keyboard_translate_scancode(scancode);
    if (ascii != 0) {
        console_write_char((uint8_t)ascii);
    }
}

static void keyboard_irq_handler(void) {
    uint8_t scancode = read_port_byte(KEYBOARD_DATA_PORT);

    keyboard_process_scancode(scancode);
}

void keyboard_init(void) {
    irq_install_handler(KEYBOARD_IRQ, keyboard_irq_handler);
    pic_clear_mask(KEYBOARD_IRQ);
}
