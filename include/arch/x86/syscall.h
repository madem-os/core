/*
 * x86 Syscall Entry Interface
 *
 * This header declares the x86-specific syscall initialization entry point.
 * It installs the syscall trap vector used by the minimal userspace ABI.
 *
 * Responsibilities in this header:
 * - expose syscall initialization for kernel bring-up
 * - keep x86 trap entry details private to arch/x86/syscall.c
 */

#ifndef ARCH_X86_SYSCALL_H
#define ARCH_X86_SYSCALL_H

void syscall_init(void);

#endif
