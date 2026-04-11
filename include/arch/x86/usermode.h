/*
 * x86 Usermode Entry Interface
 *
 * This header declares the minimal x86 helper used to transfer execution from
 * the kernel into ring 3. It is the final stage of the first usermode vertical
 * slice and assumes the syscall path and stdio wiring already exist.
 *
 * Responsibilities in this header:
 * - expose the one-shot x86 usermode entry helper
 * - keep selector values and transition mechanics private to the x86 module
 */

#ifndef ARCH_X86_USERMODE_H
#define ARCH_X86_USERMODE_H

#include <stdint.h>

void x86_enter_usermode(uint32_t entry, uint32_t user_stack_top);

#endif
