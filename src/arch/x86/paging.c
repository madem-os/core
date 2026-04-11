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

#define PAGE_SIZE 4096u
#define PAGE_TABLE_ENTRIES 1024u
#define PAGE_DIRECTORY_ENTRIES 1024u
#define IDENTITY_MAP_BYTES (16u * 1024u * 1024u)
#define IDENTITY_MAP_TABLE_COUNT (IDENTITY_MAP_BYTES / (PAGE_SIZE * PAGE_TABLE_ENTRIES))

#define PAGE_PRESENT 0x001u
#define PAGE_WRITABLE 0x002u
#define PAGE_USER 0x004u

extern void x86_load_page_directory(const void *page_directory);
extern void x86_enable_paging(void);

static uint32_t kernel_page_directory[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t kernel_page_tables[IDENTITY_MAP_TABLE_COUNT][PAGE_TABLE_ENTRIES]
    __attribute__((aligned(PAGE_SIZE)));

void paging_init_identity(void) {
    uint32_t table_index;
    uint32_t entry_index;
    uint32_t physical_address;

    for (table_index = 0; table_index < PAGE_DIRECTORY_ENTRIES; table_index++) {
        kernel_page_directory[table_index] = 0;
    }

    physical_address = 0;

    for (table_index = 0; table_index < IDENTITY_MAP_TABLE_COUNT; table_index++) {
        for (entry_index = 0; entry_index < PAGE_TABLE_ENTRIES; entry_index++) {
            kernel_page_tables[table_index][entry_index] =
                physical_address | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
            physical_address += PAGE_SIZE;
        }

        kernel_page_directory[table_index] =
            (uint32_t)kernel_page_tables[table_index] |
            PAGE_PRESENT |
            PAGE_WRITABLE |
            PAGE_USER;
    }

    x86_load_page_directory(kernel_page_directory);
    x86_enable_paging();
}
