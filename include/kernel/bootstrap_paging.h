/*
 * Bootstrap Paging Layout Helpers
 *
 * This header exposes the plain-C page-table construction helpers used by the
 * early paging bring-up path. The goal is to keep page-structure generation
 * testable in hosted unit tests while leaving privileged CR3/CR0 operations in
 * the x86-specific layer.
 *
 * Responsibilities in this header:
 * - define the fixed bootstrap paging constants used by stage 4.1
 * - expose a builder for identity-mapped bootstrap page tables
 * - keep the page-table generation policy reusable outside the x86 asm layer
 *
 * This helper does not execute privileged instructions and does not know about
 * CR3, CR0, or CPU mode transitions.
 */

#ifndef KERNEL_BOOTSTRAP_PAGING_H
#define KERNEL_BOOTSTRAP_PAGING_H

#include <stdint.h>

#define X86_PAGE_SIZE 4096u
#define X86_PAGE_TABLE_ENTRIES 1024u
#define X86_PAGE_DIRECTORY_ENTRIES 1024u

#define BOOTSTRAP_IDENTITY_MAP_BYTES (16u * 1024u * 1024u)
#define BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT \
    (BOOTSTRAP_IDENTITY_MAP_BYTES / (X86_PAGE_SIZE * X86_PAGE_TABLE_ENTRIES))

#define X86_PAGE_PRESENT 0x001u
#define X86_PAGE_WRITABLE 0x002u
#define X86_PAGE_USER 0x004u

void bootstrap_paging_build_identity(
    uint32_t *page_directory,
    uint32_t *page_tables,
    uint32_t table_count,
    uint32_t page_table_base,
    uint32_t start_physical_address,
    uint32_t page_flags
);

#endif
