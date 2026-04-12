/*
 * x86 Paging Interface
 *
 * This header declares the small paging hooks used during early higher-half
 * kernel bring-up.
 *
 * Responsibilities in this header:
 * - expose the bootstrap paging enable path
 * - expose the CR3 reload hook used by the VM layer
 * - expose the helper that drops the temporary low bootstrap alias
 * - keep CR3/CR0 details private to the x86 paging implementation
 *
 * This stage intentionally does not expose general page-mapping APIs,
 * address-space objects, or user-pointer validation.
 */

#ifndef ARCH_X86_PAGING_H
#define ARCH_X86_PAGING_H

void paging_load_page_directory(const void *page_directory);
void paging_unmap_bootstrap_low_alias(void);

#endif
