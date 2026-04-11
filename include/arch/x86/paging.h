/*
 * x86 Paging Interface
 *
 * This header declares the first paging bring-up entry point used by the
 * kernel during stage 4.1 of the virtual-memory plan.
 *
 * Responsibilities in this header:
 * - expose minimal paging initialization for the current identity-mapped
 *   bring-up step
 * - keep CR3/CR0 details private to the x86 paging implementation
 * - provide one stable entry point the kernel can call early in boot
 *
 * This stage intentionally does not expose general page-mapping APIs,
 * address-space objects, or user-pointer validation. It only turns paging on
 * in a way that preserves the current low-address execution model so the
 * existing usermode and e2e path keep working.
 */

#ifndef ARCH_X86_PAGING_H
#define ARCH_X86_PAGING_H

void paging_init_identity(void);

#endif
