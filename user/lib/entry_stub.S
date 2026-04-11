/*
 * Userspace Entry Stub
 *
 * This file provides the first instruction executed in ring 3 for built-in
 * user programs. It installs user data selectors, calls the user C entry
 * point, and loops forever if that function ever returns.
 */

    .text

    .global user_program_entry
user_program_entry:
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    call user_main
1:
    jmp 1b
