/*
 * x86 Usermode Entry Stubs
 *
 * This file owns the assembly needed to enter ring 3 and the initial usermode
 * trampoline that installs user data selectors before calling the entry point.
 */

    .text

    .global x86_enter_usermode
x86_enter_usermode:
    mov 8(%esp), %eax
    sub $4, %eax
    mov 4(%esp), %edx
    mov %edx, (%eax)

    push $0x23
    push %eax
    pushf
    pop %edx
    or $0x200, %edx
    push %edx
    push $0x1b
    push $x86_user_entry_stub
    iret
1:
    jmp 1b

    .section .user.text, "ax", @progbits
    .global x86_user_entry_stub
x86_user_entry_stub:
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    pop %eax
    call *%eax
2:
    jmp 2b
