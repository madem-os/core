/*
 * x86 Low-Level Instruction Helpers
 *
 * This header declares small architecture-only entry points that wrap
 * privileged x86 instructions awkward to express portably in C.
 *
 * Responsibilities in this header:
 * - expose the IDT load helper used during early interrupt setup
 * - expose the interrupt-enable helper used during kernel bring-up
 *
 * These helpers are intentionally tiny and policy-free. They exist only to
 * move inline assembly out of C translation units while keeping the public
 * call sites readable.
 */

#ifndef ARCH_X86_LOWLEVEL_H
#define ARCH_X86_LOWLEVEL_H

void x86_load_idt(const void *idt_ptr);
void x86_enable_interrupts(void);

#endif
