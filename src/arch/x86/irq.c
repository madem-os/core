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

__asm__(
".global irq_common_stub             \n"
"irq_common_stub:                    \n"
"    pusha                           \n"
"    mov 32(%esp), %eax              \n"
"    push %eax                       \n"
"    call irq_dispatch               \n"
"    add $4, %esp                    \n"
"    popa                            \n"
"    add $4, %esp                    \n"
"    iret                            \n"
".global irq0_stub                   \n"
"irq0_stub:                          \n"
"    push $0                         \n"
"    jmp irq_common_stub             \n"
".global irq1_stub                   \n"
"irq1_stub:                          \n"
"    push $1                         \n"
"    jmp irq_common_stub             \n"
".global irq2_stub                   \n"
"irq2_stub:                          \n"
"    push $2                         \n"
"    jmp irq_common_stub             \n"
".global irq3_stub                   \n"
"irq3_stub:                          \n"
"    push $3                         \n"
"    jmp irq_common_stub             \n"
".global irq4_stub                   \n"
"irq4_stub:                          \n"
"    push $4                         \n"
"    jmp irq_common_stub             \n"
".global irq5_stub                   \n"
"irq5_stub:                          \n"
"    push $5                         \n"
"    jmp irq_common_stub             \n"
".global irq6_stub                   \n"
"irq6_stub:                          \n"
"    push $6                         \n"
"    jmp irq_common_stub             \n"
".global irq7_stub                   \n"
"irq7_stub:                          \n"
"    push $7                         \n"
"    jmp irq_common_stub             \n"
".global irq8_stub                   \n"
"irq8_stub:                          \n"
"    push $8                         \n"
"    jmp irq_common_stub             \n"
".global irq9_stub                   \n"
"irq9_stub:                          \n"
"    push $9                         \n"
"    jmp irq_common_stub             \n"
".global irq10_stub                  \n"
"irq10_stub:                         \n"
"    push $10                        \n"
"    jmp irq_common_stub             \n"
".global irq11_stub                  \n"
"irq11_stub:                         \n"
"    push $11                        \n"
"    jmp irq_common_stub             \n"
".global irq12_stub                  \n"
"irq12_stub:                         \n"
"    push $12                        \n"
"    jmp irq_common_stub             \n"
".global irq13_stub                  \n"
"irq13_stub:                         \n"
"    push $13                        \n"
"    jmp irq_common_stub             \n"
".global irq14_stub                  \n"
"irq14_stub:                         \n"
"    push $14                        \n"
"    jmp irq_common_stub             \n"
".global irq15_stub                  \n"
"irq15_stub:                         \n"
"    push $15                        \n"
"    jmp irq_common_stub             \n"
);
