/*
 * x86 Port I/O Primitives
 *
 * This file owns the raw x86 port access helpers used by architecture
 * support code and low-level drivers that communicate through I/O ports.
 *
 * Responsibilities in this file:
 * - provide minimal wrappers around inb/outb/outw
 * - expose a tiny I/O wait primitive
 *
 * The interface is intentionally tiny and policy-free.
 */

    .text

    .global write_port_byte
write_port_byte:
    mov 4(%esp), %dx
    mov 8(%esp), %al
    outb %al, %dx
    ret

    .global read_port_byte
read_port_byte:
    mov 4(%esp), %dx
    inb %dx, %al
    movzbl %al, %eax
    ret

    .global write_port_word
write_port_word:
    mov 4(%esp), %dx
    mov 8(%esp), %ax
    outw %ax, %dx
    ret

    .global io_wait
io_wait:
    xor %eax, %eax
    outb %al, $0x80
    ret
