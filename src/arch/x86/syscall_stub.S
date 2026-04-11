/*
 * x86 Syscall Assembly Stub
 *
 * This file owns the interrupt entry stub used by the `int 0x80` syscall
 * path. It preserves registers, marshals arguments, and returns with iret.
 */

    .text

    .extern x86_syscall_handle

    .global x86_syscall_stub
x86_syscall_stub:
    pusha

    xor %eax, %eax
    mov %ds, %ax
    push %eax
    xor %eax, %eax
    mov %es, %ax
    push %eax
    xor %eax, %eax
    mov %fs, %ax
    push %eax
    xor %eax, %eax
    mov %gs, %ax
    push %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov 44(%esp), %eax
    mov 32(%esp), %ebx
    mov 40(%esp), %ecx
    mov 36(%esp), %edx
    push %edx
    push %ecx
    push %ebx
    push %eax
    call x86_syscall_handle
    add $16, %esp
    mov %eax, 44(%esp)

    pop %eax
    mov %ax, %gs
    pop %eax
    mov %ax, %fs
    pop %eax
    mov %ax, %es
    pop %eax
    mov %ax, %ds

    popa
    iret
