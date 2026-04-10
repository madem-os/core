/*
 * x86 IRQ Dispatcher Interface
 *
 * This header defines how higher-level code and device drivers interact with
 * the legacy IRQ dispatcher installed on top of the remapped PIC vectors.
 *
 * Responsibilities in this header:
 * - define the callback type used for IRQ handlers
 * - expose dispatcher initialization
 * - expose handler install and uninstall entry points
 *
 * Drivers register callbacks here, while the implementation retains ownership
 * of assembly stubs, IDT placement, and PIC end-of-interrupt signaling.
 */

#ifndef ARCH_X86_IRQ_H
#define ARCH_X86_IRQ_H

#include <stdint.h>

typedef void (*irq_handler_t)(void);

void irq_init(void);
void irq_install_handler(uint8_t irq, irq_handler_t handler);
void irq_uninstall_handler(uint8_t irq);

#endif
