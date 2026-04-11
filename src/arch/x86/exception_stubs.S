/*
 * x86 Exception Assembly Stubs
 *
 * This file owns the assembly entry points for synchronous CPU exceptions that
 * the kernel currently handles explicitly. The first handler implemented here
 * is page fault (#PF), which pushes a CPU-provided error code.
 *
 * Responsibilities in this file:
 * - save general-purpose and segment registers before entering C
 * - switch to kernel data segments before calling the C handler
 * - pass a stable stack-frame pointer to the page-fault handler
 * - restore the interrupted state and return with iret
 *
 * The current page-fault handler does not return in practice, but the stub is
 * written as a normal trap path so later recovery work has a clean base.
 */

    .text

    .extern page_fault_handle

    .global page_fault_stub
page_fault_stub:
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

    push %esp
    call page_fault_handle
    add $4, %esp

    pop %eax
    mov %ax, %gs
    pop %eax
    mov %ax, %fs
    pop %eax
    mov %ax, %es
    pop %eax
    mov %ax, %ds

    popa
    add $4, %esp
    iret
