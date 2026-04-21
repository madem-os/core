/*
 * Kernel Virtual Memory Interface
 *
 * This header declares the first isolated-user-mapping VM layer. It keeps the
 * VM layout policy and page-table construction logic in plain C so it can be
 * unit-tested, while the privileged CR3 work remains in the x86 paging layer.
 *
 * Responsibilities in this header:
 * - define the fixed user virtual-address contract for stage 4.3
 * - declare the process-owned VM descriptor
 * - expose testable helpers for page-count and layout calculations
 * - expose the runtime helpers that prepare and activate one isolated process
 *
 * This stage is intentionally minimal. It supports only one built-in user
 * image, one process-owned page directory, and one low user stack.
 */

#ifndef KERNEL_VM_H
#define KERNEL_VM_H

#include <stddef.h>
#include <stdint.h>

#include "kernel/bootstrap_paging.h"

/* Base virtual address of the shared higher-half kernel mapping. */
#define VM_KERNEL_VIRT_BASE         0xC0000000u
/* Total higher-half kernel window installed into each process page directory. */
#define VM_KERNEL_HIGH_MAP_BYTES    (16u * 1024u * 1024u)

/* Lowest user virtual address reserved for the first isolated process. */
#define USER_BASE                   0x00400000u
/* Base virtual address where the copied built-in user image is mapped. */
#define USER_TEXT_BASE              0x00400000u
/* Reserved future heap base for the first user address-space contract. */
#define USER_HEAP_BASE              0x00800000u
/* Top of the first user stack mapping. */
#define USER_STACK_TOP              0x00C00000u
/* Exclusive upper bound of the user virtual window. */
#define USER_LIMIT                  0x00C00000u

/* Number of initially mapped user stack pages. */
#define VM_USER_STACK_PAGES         1u
/* Number of guard pages reserved immediately below the user stack. */
#define VM_USER_STACK_GUARD_PAGES   1u

struct process;

/* Converts a kernel virtual pointer used by the VM builder into a physical address. */
typedef uint32_t (*vm_pointer_to_physical_fn)(const void *pointer);
/* Activates a process page directory once the VM layer has finished building it. */
typedef void (*vm_load_page_directory_fn)(const void *page_directory_physical);

struct vm_space {
    /* Inclusive lower bound of the process user virtual range. */
    uintptr_t user_base;
    /* Exclusive upper bound of the process user virtual range. */
    uintptr_t user_limit;
    /* Virtual base address where the process text image is mapped. */
    uintptr_t text_base;
    /* Total in-memory size of the copied user image, including zero-filled tail. */
    size_t text_size;
    /* Virtual top of the mapped user stack region. */
    uintptr_t stack_top;
    /* Total mapped user stack size in bytes. */
    size_t stack_size;
    /* Page directory currently owning this process address space. */
    uint32_t *page_directory;
};

struct vm_user_image {
    /* Kernel-visible source bytes that should be copied into user text pages. */
    const uint8_t *load_start;
    /* Number of initialized image bytes to copy from load_start. */
    size_t load_size;
    /* Total in-memory size the image should occupy after zero-fill. */
    size_t memory_size;
    /* Entry-point offset relative to USER_TEXT_BASE inside the copied image. */
    uintptr_t entry_offset;
};

struct vm_runtime {
    /* Page directory backing the process address space being built. */
    uint32_t *page_directory;
    /* Higher-half kernel page tables reused in the process CR3. */
    uint32_t *kernel_high_page_tables;
    /* Number of higher-half kernel page tables available through kernel_high_page_tables. */
    uint32_t kernel_high_table_count;
    /* Page table used for the low user text mapping. */
    uint32_t *user_text_page_table;
    /* Page table used for the low user stack mapping. */
    uint32_t *user_stack_page_table;
    /* Page-sized backing store used by the stage-4.3 bump allocator. */
    uint8_t *frame_pool;
    /* Total number of page frames available in frame_pool. */
    size_t frame_pool_page_count;
    /* Current page-frame allocation cursor inside frame_pool. */
    size_t next_free_frame_page;
    /* Platform hook that converts runtime pointers to physical addresses. */
    vm_pointer_to_physical_fn pointer_to_physical;
    /* Platform hook that makes a process page directory active. */
    vm_load_page_directory_fn load_page_directory;
};

/* Parses a built-in ELF user program into the compact vm_user_image shape. */
int vm_user_image_from_elf(
    struct vm_user_image *user_image,
    const uint8_t *elf_bytes,
    size_t elf_size
);

/* Returns how many 4 KiB pages are required to cover byte_count bytes. */
size_t vm_page_count(size_t byte_count);
/* Returns the base address of the guard page immediately below a user stack region. */
uintptr_t vm_stack_guard_base(uintptr_t stack_top, size_t stack_size);
/* Initializes a vm_space with the fixed stage-4.3 user address-space contract. */
void vm_space_init(struct vm_space *space, size_t user_text_size);
/*
 * Populates a process page directory with:
 * - shared higher-half kernel mappings
 * - one user text mapping
 * - one user stack mapping
 */
void vm_build_process_page_tables(
    uint32_t *page_directory,
    uint32_t *kernel_high_page_tables,
    uint32_t *user_text_page_table,
    uint32_t *user_stack_page_table,
    const struct vm_space *space,
    vm_pointer_to_physical_fn pointer_to_physical,
    uint32_t kernel_high_table_count,
    uint32_t user_text_physical_base,
    uint32_t user_stack_physical_base
);
/* Allocates user backing pages, copies the user image, and prepares process entry/stack state. */
int vm_init_process(
    struct process *process,
    struct vm_runtime *runtime,
    const struct vm_user_image *user_image
);
/* Activates the prepared process page directory through the runtime paging hook. */
void vm_activate_process(
    const struct process *process,
    const struct vm_runtime *runtime
);
/* Resets transient allocation state in a runtime before rebuilding an address space. */
void vm_reset_runtime(struct vm_runtime *runtime);
/* Returns an inactive runtime buffer suitable for building the next process image. */
struct vm_runtime *vm_prepare_next_runtime(void);
/* Marks a prepared runtime as the active process runtime after a successful switch. */
void vm_set_active_runtime(struct vm_runtime *runtime);
/* Returns the default kernel-owned VM runtime buffers and platform hooks. */
struct vm_runtime *vm_default_runtime(void);
#endif
