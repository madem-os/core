/*
 * Kernel Syscall Dispatch
 *
 * This file implements the generic syscall dispatcher used above the x86 trap
 * entry path. It decodes syscall numbers and routes them to the kernel I/O
 * layer introduced during the first usermode-preparation stage.
 *
 * Responsibilities in this file:
 * - define the first syscall dispatch behavior
 * - route SYS_READ to kread
 * - route SYS_WRITE to kwrite
 * - reject unsupported syscall numbers
 *
 * This file should remain generic and should not depend on IDT setup, register
 * save/restore mechanics, or other x86 entry details.
 */

#include "kernel/io.h"
#include "kernel/syscall.h"

int syscall_dispatch(
    kernel_syscall_arg_t number,
    kernel_syscall_arg_t arg1,
    kernel_syscall_arg_t arg2,
    kernel_syscall_arg_t arg3
) {
    switch (number) {
        case SYS_READ:
            return kread((int)arg1, (char *)arg2, (int)arg3);
        case SYS_WRITE:
            return kwrite((int)arg1, (const char *)arg2, (int)arg3);
        default:
            return -1;
    }
}
