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
    mov 28(%esp), %eax
    mov 16(%esp), %ebx
    mov 24(%esp), %ecx
    mov 20(%esp), %edx
    push %edx
    push %ecx
    push %ebx
    push %eax
    call x86_syscall_handle
    add $16, %esp
    mov %eax, 28(%esp)
    popa
    iret
