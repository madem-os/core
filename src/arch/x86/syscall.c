/*
 * x86 Syscall Entry
 *
 * This file installs and implements the first x86 syscall entry path using
 * `int 0x80`. It bridges register arguments from the trap frame into the
 * generic kernel syscall dispatcher.
 *
 * Responsibilities in this file:
 * - define the syscall vector number
 * - install the syscall gate in the IDT
 * - provide the assembly stub entered by `int 0x80`
 * - forward register arguments to the generic dispatcher
 *
 * This file should not contain fd routing or TTY policy. Those belong to the
 * generic kernel syscall and I/O layers above it.
 */

#include <stdint.h>

#include "arch/x86/idt.h"
#include "arch/x86/syscall.h"
#include "kernel/syscall.h"

#define SYSCALL_VECTOR 0x80u
#define IDT_USER_TRAP_GATE 0xEFu

kernel_syscall_arg_t x86_syscall_handle(
    kernel_syscall_arg_t number,
    kernel_syscall_arg_t arg1,
    kernel_syscall_arg_t arg2,
    kernel_syscall_arg_t arg3
) {
    return (kernel_syscall_arg_t)syscall_dispatch(number, arg1, arg2, arg3);
}

extern void x86_syscall_stub(void);

void syscall_init(void) {
    idt_set_gate_with_type_attr(
        SYSCALL_VECTOR,
        (uint32_t)x86_syscall_stub,
        IDT_USER_TRAP_GATE
    );
}
