/*
 * x86 Paging Bring-Up
 *
 * This file owns the small x86 paging hooks used during early higher-half
 * kernel bring-up.
 *
 * Responsibilities in this file:
 * - define the original bootstrap identity-mapped page directory and tables
 * - load CR3 and enable paging in CR0
 * - reload CR3 for later VM activations
 * - remove the temporary low bootstrap alias once the higher-half kernel is
 *   fully self-hosted
 *
 * The bootstrap identity mapping still exists here because it is part of the
 * original bring-up path, but the normal runtime path now switches to the
 * higher half and then drops the low alias explicitly.
 */

#include <stdint.h>

#include "arch/x86/paging.h"
#include "kernel/bootstrap_paging.h"

extern void x86_load_page_directory(const void *page_directory);
extern void x86_enable_paging(void);

#define BOOTSTRAP_PAGE_DIRECTORY_PHYS 0x00020000u
#define VM_KERNEL_VIRT_BASE 0xC0000000u

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

void paging_load_page_directory(const void *page_directory) {
    x86_load_page_directory(page_directory);
}

void paging_unmap_bootstrap_low_alias(void) {
    uint32_t *bootstrap_page_directory;

    bootstrap_page_directory =
        (uint32_t *)(uintptr_t)(VM_KERNEL_VIRT_BASE + BOOTSTRAP_PAGE_DIRECTORY_PHYS);

    bootstrap_page_directory[0] = 0;
    x86_load_page_directory((const void *)(uintptr_t)BOOTSTRAP_PAGE_DIRECTORY_PHYS);
}
