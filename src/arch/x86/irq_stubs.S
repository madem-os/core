/*
 * x86 IRQ Assembly Stubs
 *
 * This file owns the assembly entry points for the 16 legacy PIC IRQ lines
 * and the shared transition into the C dispatcher in irq.c.
 *
 * Responsibilities in this file:
 * - provide one distinct IDT entry point per PIC IRQ line
 * - save and restore general-purpose and segment registers
 * - switch to the kernel data segments before calling C code
 * - pass the IRQ number to irq_dispatch()
 * - return from the interrupt with iret
 *
 * This file intentionally does not know about IDT registration policy,
 * handler ownership, or PIC EOI rules. Those stay in the C IRQ layer.
 */

    .text

    .extern irq_dispatch

    .global irq_common_stub
    
irq_common_stub:
    pusha

    xor %eax, %eax
    mov %ds, %ax
    push %eax
    xor %eax, %eax
    mov %es, %ax
    push %eax
    xor %eax, %eax
    mov %fs, %ax
    push %eax
    xor %eax, %eax
    mov %gs, %ax
    push %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov 48(%esp), %eax
    push %eax
    call irq_dispatch
    add $4, %esp

    pop %eax
    mov %ax, %gs
    pop %eax
    mov %ax, %fs
    pop %eax
    mov %ax, %es
    pop %eax
    mov %ax, %ds

    popa
    add $4, %esp
    iret

    .macro IRQ_STUB irq_number
    .global irq\irq_number\()_stub
irq\irq_number\()_stub:
    push $\irq_number
    jmp irq_common_stub
    .endm

    IRQ_STUB 0
    IRQ_STUB 1
    IRQ_STUB 2
    IRQ_STUB 3
    IRQ_STUB 4
    IRQ_STUB 5
    IRQ_STUB 6
    IRQ_STUB 7
    IRQ_STUB 8
    IRQ_STUB 9
    IRQ_STUB 10
    IRQ_STUB 11
    IRQ_STUB 12
    IRQ_STUB 13
    IRQ_STUB 14
    IRQ_STUB 15
