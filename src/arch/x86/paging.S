/*
 * x86 Paging Low-Level Helpers
 *
 * This file contains the privileged instruction wrappers needed for the first
 * paging enable step.
 *
 * Responsibilities in this file:
 * - load CR3 with the active page directory base
 * - enable paging by setting CR0.PG
 *
 * These helpers stay intentionally tiny so the paging policy remains in the C
 * implementation.
 */

    .text

    .global x86_load_page_directory
x86_load_page_directory:
    mov 4(%esp), %eax
    mov %eax, %cr3
    ret

    .global x86_enable_paging
x86_enable_paging:
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    ret
