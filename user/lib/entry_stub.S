/*
 * Userspace Entry Stub
 *
 * This file provides the first instruction executed in ring 3 for built-in
 * user programs. It installs user data selectors, calls `main(argc, argv)`
 * with `0, NULL` for now, and loops forever if that function ever returns.
 */

    .text

    .global user_program_entry
user_program_entry:
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    push $0
    push $0
    call main
    add $8, %esp
1:
    jmp 1b
