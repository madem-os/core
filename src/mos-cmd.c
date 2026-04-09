#include <stdint.h>
#include <stdbool.h>

#include "mos-cmd.h"
#include "mos-ports.h"
#include "mos-stdio.h"

struct IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IDTPtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct IDTEntry idt[256];
static struct IDTPtr idtp;

void set_idt_gate(int n, uint32_t handler) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x08;
    idt[n].zero        = 0;
    idt[n].type_attr   = 0x8E;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void load_idt() {
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;
    asm volatile("lidt %0" : : "m"(idtp));
}

void remap_pic() {
    write_port_byte(0x20, 0x11);
    io_wait();
    write_port_byte(0xA0, 0x11);
    io_wait();

    write_port_byte(0x21, 0x20); // master offset = 0x20
    io_wait();
    write_port_byte(0xA1, 0x28); // slave offset = 0x28
    io_wait();

    write_port_byte(0x21, 0x04);
    io_wait();
    write_port_byte(0xA1, 0x02);
    io_wait();

    write_port_byte(0x21, 0x01);
    io_wait();
    write_port_byte(0xA1, 0x01);
    io_wait();

    // Enable only keyboard IRQ1 for now
    write_port_byte(0x21, 0xFD);
    io_wait();
    write_port_byte(0xA1, 0xFF);
    io_wait();
}

void keyboard_handler();

extern void keyboard_handler_stub(void);

__asm__ (
".global keyboard_handler_stub    \n"
"keyboard_handler_stub:           \n"
"    pusha                        \n"
"    call keyboard_handler        \n"
"    popa                         \n"
"    iret                         \n"
);

void keyboard_handler() {
    uint8_t scancode = read_port_byte(0x60);
    process_scancode(scancode);
    write_port_byte(0x20, 0x20); // EOI for master PIC
}

void init_idt() {
    for (int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }

    set_idt_gate(0x21, (uint32_t)keyboard_handler_stub);
}

void command_line_mode() {
    print_line("you are now in command-line mode!");

    remap_pic();
    init_idt();
    load_idt();

    asm volatile("sti");

    print_line("interrupts enabled!");

    while (1) {
        asm volatile("hlt");
    }
}