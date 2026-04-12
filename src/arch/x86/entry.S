/*
 * x86 Kernel Entry Stub
 *
 * This file owns the earliest C-visible kernel entry point reached after the
 * bootloader transfers control into the linked kernel image.
 *
 * Responsibilities in this file:
 * - establish the expected segment registers for the kernel
 * - install the first kernel stack
 * - transfer control into kmain()
 * - halt in place if kmain() ever returns
 *
 * The symbol defined here is referenced by the boot assembly and intentionally
 * stays outside the regular C modules so the entry sequence is explicit and
 * free of inline assembly.
 */

    .section .text.entry, "ax", @progbits

    .extern kmain

    .global kernel_entry
kernel_entry:
    cli
    cld
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    mov $0xC0090000, %esp
    call kmain
1:
    hlt
    jmp 1b
