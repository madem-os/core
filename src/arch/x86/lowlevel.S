/*
 * x86 Low-Level Instruction Helpers
 *
 * This file contains small wrappers around privileged x86 instructions used
 * during early kernel bring-up.
 *
 * Responsibilities in this file:
 * - load the active IDT with lidt
 * - enable interrupts with sti
 * - read CR2 for page-fault reporting
 * - halt forever in fatal paths
 *
 * These helpers intentionally do not own IRQ policy, syscall dispatch, or any
 * higher-level kernel logic.
 */

    .text

    .global x86_load_idt
x86_load_idt:
    mov 4(%esp), %eax
    lidt (%eax)
    ret

    .global x86_load_gdt
x86_load_gdt:
    mov 4(%esp), %eax
    lgdt (%eax)
    ret

    .global x86_load_task_register
x86_load_task_register:
    mov 4(%esp), %ax
    ltr %ax
    ret

    .global x86_enable_interrupts
x86_enable_interrupts:
    sti
    ret

    .global x86_read_cr2
x86_read_cr2:
    mov %cr2, %eax
    ret

    .global x86_halt_forever
x86_halt_forever:
    cli
1:
    hlt
    jmp 1b
