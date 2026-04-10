#ifndef ARCH_X86_IDT_H
#define ARCH_X86_IDT_H

#include <stdint.h>

void idt_init(void);
void idt_set_gate(uint8_t vector, uint32_t handler);

#endif
