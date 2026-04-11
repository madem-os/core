/*
 * x86 Paging Bring-Up
 *
 * This file owns the first paging enable step for the kernel. The current
 * stage uses a simple identity-mapped page-table setup so paging can be
 * enabled without changing any virtual addresses yet.
 *
 * Responsibilities in this file:
 * - define the bootstrap page directory and page tables
 * - identity-map the low memory range currently used by the kernel
 * - load CR3 and enable paging in CR0
 *
 * The mappings here are intentionally broad and user-accessible because the
 * current ring-3 program still executes from the kernel-linked low addresses.
 * This stage preserves behavior, not protection. Higher-half kernel mapping
 * and isolated user mappings are later steps.
 */

#include <stdint.h>

#include "arch/x86/paging.h"
#include "kernel/bootstrap_paging.h"

extern void x86_load_page_directory(const void *page_directory);
extern void x86_enable_paging(void);

static uint32_t kernel_page_directory[X86_PAGE_DIRECTORY_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint32_t kernel_page_tables[BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT][X86_PAGE_TABLE_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));

void paging_init_identity(void) {
    bootstrap_paging_build_identity(
        kernel_page_directory,
        &kernel_page_tables[0][0],
        BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT,
        (uint32_t)(uintptr_t)&kernel_page_tables[0][0],
        0,
        X86_PAGE_PRESENT | X86_PAGE_WRITABLE | X86_PAGE_USER
    );

    x86_load_page_directory(kernel_page_directory);
    x86_enable_paging();
}
