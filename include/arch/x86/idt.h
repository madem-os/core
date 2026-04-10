/*
 * x86 Interrupt Descriptor Table Interface
 *
 * This header declares the minimal IDT operations needed by the rest of the
 * kernel to initialize the table and install handler entry points.
 *
 * Responsibilities in this header:
 * - expose whole-table initialization for early boot
 * - expose per-vector gate installation
 *
 * The layout details of descriptor entries stay private to idt.c. Callers
 * should treat this as a narrow architecture support interface.
 */

#ifndef ARCH_X86_IDT_H
#define ARCH_X86_IDT_H

#include <stdint.h>

void idt_init(void);
void idt_set_gate(uint8_t vector, uint32_t handler);

#endif
