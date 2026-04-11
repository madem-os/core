/*
 * Bootstrap Paging Layout Helpers
 *
 * This file contains the plain-C page-table construction logic used by the
 * early paging bring-up path. It intentionally contains no privileged x86
 * instructions so that the layout policy can be validated in hosted tests.
 *
 * Responsibilities in this file:
 * - clear the bootstrap page directory
 * - build one or more identity-mapped page tables
 * - populate PDEs that reference a contiguous page-table block
 *
 * The caller is responsible for allocating aligned buffers and for actually
 * loading CR3 / enabling paging through the x86-specific layer.
 */

#include <stdint.h>

#include "kernel/bootstrap_paging.h"

void bootstrap_paging_build_identity(
    uint32_t *page_directory,
    uint32_t *page_tables,
    uint32_t table_count,
    uint32_t page_table_base,
    uint32_t start_physical_address,
    uint32_t page_flags
) {
    uint32_t directory_index;
    uint32_t table_index;
    uint32_t entry_index;
    uint32_t physical_address;

    for (directory_index = 0; directory_index < X86_PAGE_DIRECTORY_ENTRIES; directory_index++) {
        page_directory[directory_index] = 0;
    }

    physical_address = start_physical_address;

    for (table_index = 0; table_index < table_count; table_index++) {
        for (entry_index = 0; entry_index < X86_PAGE_TABLE_ENTRIES; entry_index++) {
            page_tables[(table_index * X86_PAGE_TABLE_ENTRIES) + entry_index] =
                physical_address | page_flags;
            physical_address += X86_PAGE_SIZE;
        }

        page_directory[table_index] =
            (page_table_base + (table_index * X86_PAGE_SIZE)) | page_flags;
    }
}
