#include <stdint.h>

#include "arch/x86/irq.h"
#include "arch/x86/pic.h"
#include "drivers/keyboard.h"
#include "mos-ports.h"
#include "mos-stdio.h"

#define KEYBOARD_IRQ 1
#define KEYBOARD_DATA_PORT 0x60

static void keyboard_irq_handler(void) {
    uint8_t scancode = read_port_byte(KEYBOARD_DATA_PORT);

    process_scancode(scancode);
}

void keyboard_init(void) {
    irq_install_handler(KEYBOARD_IRQ, keyboard_irq_handler);
    pic_clear_mask(KEYBOARD_IRQ);
}
