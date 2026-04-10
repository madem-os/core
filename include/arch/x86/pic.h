/*
 * x86 Legacy PIC Interface
 *
 * This header declares the small control surface for the dual 8259 PIC used
 * by the current kernel during early interrupt bring-up.
 *
 * Responsibilities in this header:
 * - expose PIC remapping and initialization
 * - expose per-IRQ mask control
 * - expose end-of-interrupt signaling
 *
 * Consumers should use these helpers rather than reaching for raw PIC ports
 * directly so interrupt policy remains centralized in pic.c.
 */

#ifndef ARCH_X86_PIC_H
#define ARCH_X86_PIC_H

#include <stdint.h>

void pic_init(void);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);
void pic_send_eoi(uint8_t irq);

#endif
