/*
 * Userspace Syscall Assembly Helper
 *
 * This file owns the tiny helper used by the libc-like read/write wrappers to
 * issue `int 0x80` with three arguments.
 */

    .section .user.text, "ax", @progbits

    .global user_syscall3
user_syscall3:
    push %ebx
    mov 8(%esp), %eax
    mov 12(%esp), %ebx
    mov 16(%esp), %ecx
    mov 20(%esp), %edx
    int $0x80
    pop %ebx
    ret
