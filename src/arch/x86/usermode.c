/*
 * x86 Usermode Entry
 *
 * This file implements the one-shot ring 3 transition used by the first
 * built-in userspace process. It prepares an `iret` frame and jumps into a
 * small ring 3 trampoline that loads user data selectors and calls `user_main`.
 *
 * Responsibilities in this file:
 * - prepare the initial ring 3 stack
 * - build the `iret` frame with user selectors
 * - enter ring 3 with interrupts enabled
 *
 * This file intentionally does not know about TTY policy or syscall dispatch.
 * It only transfers control to the userspace entry point.
 */

#include <stdint.h>

#include "arch/x86/usermode.h"

#define USER_CODE_SELECTOR 0x1Bu
#define USER_DATA_SELECTOR 0x23u

extern void x86_user_entry_stub(void);

__attribute__((noreturn)) void x86_enter_usermode(
    uint32_t entry,
    uint32_t user_stack_top
) {
    uint32_t *stack;

    stack = (uint32_t *)user_stack_top;
    stack--;
    *stack = entry;

    __asm__ volatile (
        "pushl %[user_ss]\n"
        "pushl %[user_esp]\n"
        "pushfl\n"
        "popl %%eax\n"
        "orl $0x200, %%eax\n"
        "pushl %%eax\n"
        "pushl %[user_cs]\n"
        "pushl %[entry_stub]\n"
        "iret\n"
        :
        : [user_ss] "i" (USER_DATA_SELECTOR),
          [user_esp] "r" ((uint32_t)stack),
          [user_cs] "i" (USER_CODE_SELECTOR),
          [entry_stub] "r" ((uint32_t)x86_user_entry_stub)
        : "eax", "memory"
    );

    for (;;) {
    }
}

__asm__(
".global x86_user_entry_stub          \n"
"x86_user_entry_stub:                 \n"
"    mov $0x23, %ax                   \n"
"    mov %ax, %ds                     \n"
"    mov %ax, %es                     \n"
"    mov %ax, %fs                     \n"
"    mov %ax, %gs                     \n"
"    pop %eax                         \n"
"    call *%eax                       \n"
"1:                                   \n"
"    jmp 1b                           \n"
);
