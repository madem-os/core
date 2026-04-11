/*
 * x86 Usermode Entry Stubs
 *
 * This file owns the assembly needed to enter ring 3. The first instruction
 * executed in ring 3 now lives inside the separately linked user ELF image,
 * so this file only performs the privilege transition itself.
 */

    .text

    .global x86_enter_usermode
x86_enter_usermode:
    mov 4(%esp), %eax
    mov 8(%esp), %edx
    push $0x23
    push %edx
    pushf
    pop %edx
    or $0x200, %edx
    push %edx
    push $0x1b
    push %eax
    iret
1:
    jmp 1b
