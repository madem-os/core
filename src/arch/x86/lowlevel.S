/*
 * x86 Low-Level Instruction Helpers
 *
 * This file contains small wrappers around privileged x86 instructions used
 * during early kernel bring-up.
 *
 * Responsibilities in this file:
 * - load the active IDT with lidt
 * - enable interrupts with sti
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

    .global x86_enable_interrupts
x86_enable_interrupts:
    sti
    ret
