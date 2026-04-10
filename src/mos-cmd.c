#include "arch/x86/idt.h"
#include "arch/x86/irq.h"
#include "arch/x86/pic.h"
#include "drivers/keyboard.h"
#include "mos-cmd.h"
#include "mos-stdio.h"

void command_line_mode() {
    idt_init();
    pic_init();
    irq_init();
    keyboard_init();

    asm volatile("sti");

    print_line("interrupts ready");

    while (1) {
        asm volatile("hlt");
    }
}
