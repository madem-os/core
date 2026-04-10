/*
 * x86 Interrupt Descriptor Table Setup
 *
 * This file owns the in-memory IDT structure and the minimal routines needed
 * to populate descriptor entries and load the table with lidt.
 *
 * Responsibilities in this file:
 * - define the packed IDT entry and pointer layouts
 * - clear the IDT during early boot
 * - install handler addresses into specific vectors
 * - load the active IDT pointer for the current CPU
 *
 * This file should not decide which device driver handles which IRQ, and it
 * should not contain PIC policy or keyboard code. Those concerns belong to
 * the IRQ dispatcher, PIC module, and drivers.
 *
 * The implementation assumes a 32-bit protected-mode kernel using the usual
 * code segment selector at 0x08 for interrupt gates.
 */

#include <stdint.h>

#include "arch/x86/idt.h"

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
static struct IDTPtr idt_ptr;

void idt_set_gate(uint8_t vector, uint32_t handler) {
    idt[vector].offset_low = handler & 0xFFFF;
    idt[vector].selector = 0x08;
    idt[vector].zero = 0;
    idt[vector].type_attr = 0x8E;
    idt[vector].offset_high = (handler >> 16) & 0xFFFF;
}

void idt_init(void) {
    for (int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;

    asm volatile("lidt %0" : : "m"(idt_ptr));
}
