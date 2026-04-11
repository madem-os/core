/*
 * x86 IRQ Registration And Dispatch
 *
 * This file owns the mapping from hardware IRQ lines to IDT vectors and the
 * dispatch path that invokes installed C handlers when an IRQ fires.
 *
 * Responsibilities in this file:
 * - store one C callback per legacy PIC IRQ line
 * - install IRQ stubs into the IDT at the remapped vector range
 * - dispatch from the assembly stub into the registered handler
 * - send end-of-interrupt notifications through the PIC module
 *
 * Device drivers should register handlers here, but they should not send
 * EOIs directly. Raw IDT entry details remain in idt.c, while PIC signaling
 * policy remains in pic.c.
 *
 * The current implementation targets the 16 legacy PIC IRQ lines beginning
 * at vector 0x20 after remapping.
 */

#include <stdint.h>

#include "arch/x86/idt.h"
#include "arch/x86/irq.h"
#include "arch/x86/pic.h"

#define IRQ_COUNT 16
#define IRQ_VECTOR_BASE 0x20

static irq_handler_t irq_handlers[IRQ_COUNT];

void irq_dispatch(uint32_t irq) {
    if (irq < IRQ_COUNT && irq_handlers[irq] != 0) {
        irq_handlers[irq]();
    }

    pic_send_eoi((uint8_t)irq);
}

void irq_install_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < IRQ_COUNT) {
        irq_handlers[irq] = handler;
    }
}

void irq_uninstall_handler(uint8_t irq) {
    if (irq < IRQ_COUNT) {
        irq_handlers[irq] = 0;
    }
}

extern void irq0_stub(void);
extern void irq1_stub(void);
extern void irq2_stub(void);
extern void irq3_stub(void);
extern void irq4_stub(void);
extern void irq5_stub(void);
extern void irq6_stub(void);
extern void irq7_stub(void);
extern void irq8_stub(void);
extern void irq9_stub(void);
extern void irq10_stub(void);
extern void irq11_stub(void);
extern void irq12_stub(void);
extern void irq13_stub(void);
extern void irq14_stub(void);
extern void irq15_stub(void);

static void (*const irq_stubs[IRQ_COUNT])(void) = {
    irq0_stub, irq1_stub, irq2_stub, irq3_stub,
    irq4_stub, irq5_stub, irq6_stub, irq7_stub,
    irq8_stub, irq9_stub, irq10_stub, irq11_stub,
    irq12_stub, irq13_stub, irq14_stub, irq15_stub,
};

void irq_init(void) {
    for (uint8_t irq = 0; irq < IRQ_COUNT; irq++) {
        irq_handlers[irq] = 0;
        idt_set_gate((uint8_t)(IRQ_VECTOR_BASE + irq), (uint32_t)irq_stubs[irq]);
    }
}
