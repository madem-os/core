/*
 * x86 Exception Dispatcher Interface
 *
 * This header declares the minimal exception setup used by the current kernel
 * to install synchronous CPU fault handlers, starting with page faults.
 *
 * Responsibilities in this header:
 * - expose whole-subsystem initialization for exception vectors
 * - keep x86 trap-frame details private to the exception implementation
 *
 * This layer is intentionally separate from PIC IRQ dispatch because CPU
 * exceptions are not hardware IRQs and have different stack-frame rules.
 */

#ifndef ARCH_X86_EXCEPTIONS_H
#define ARCH_X86_EXCEPTIONS_H

/* Installs the current CPU exception handlers into the IDT. */
void exceptions_init(void);

#endif
