#ifndef ARCH_X86_IRQ_H
#define ARCH_X86_IRQ_H

#include <stdint.h>

typedef void (*irq_handler_t)(void);

void irq_init(void);
void irq_install_handler(uint8_t irq, irq_handler_t handler);
void irq_uninstall_handler(uint8_t irq);

#endif
