/*
 * x86 Paging Bring-Up
 *
 * This file owns the remaining small x86 paging hooks used after the bootstrap
 * paging transition has already happened in the boot path.
 *
 * Responsibilities in this file:
 * - reload CR3 for later VM activations
 * - remove the temporary low bootstrap alias once the higher-half kernel is
 *   fully self-hosted
 *
 * It intentionally does not build page tables or enable paging. Those steps
 * now belong to the bootloader/bootstrap path.
 */

#include <stdint.h>

#include "arch/x86/paging.h"

extern void x86_load_page_directory(const void *page_directory);

#define BOOTSTRAP_PAGE_DIRECTORY_PHYS 0x00020000u
#define VM_KERNEL_VIRT_BASE 0xC0000000u

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
