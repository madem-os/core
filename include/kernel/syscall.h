/*
 * Kernel Syscall Dispatch Interface
 *
 * This header declares the generic syscall dispatcher used above the x86 trap
 * entry layer. It maps syscall numbers and arguments onto the kernel I/O layer
 * without depending on IDT or register-save details.
 *
 * Responsibilities in this header:
 * - define the first syscall numbers
 * - define the pointer-sized syscall argument type
 * - expose the generic dispatcher entry point
 *
 * The first syscall ABI is intentionally tiny and only supports read/write.
 */

#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <stdint.h>

enum syscall_number {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_EXECVE = 2,
};

typedef intptr_t kernel_syscall_arg_t;

int syscall_dispatch(
    kernel_syscall_arg_t number,
    kernel_syscall_arg_t arg1,
    kernel_syscall_arg_t arg2,
    kernel_syscall_arg_t arg3
);

#endif
