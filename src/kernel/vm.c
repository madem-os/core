/*
 * Kernel VM Policy And Process Mapping
 *
 * This file owns the first isolated-user-mapping implementation. It keeps the
 * virtual-memory layout and page-table construction logic in plain C so it can
 * be unit-tested, while using the x86 paging layer only when the active page
 * directory must actually be switched.
 *
 * Responsibilities in this file:
 * - define the fixed low user-address contract for stage 4.3
 * - build a process-owned page directory with shared kernel mappings
 * - allocate backing pages for the copied user image and user stack
 * - copy the built-in user image into its user-mapped physical pages
 * - activate the process page directory before entering ring 3
 *
 * This stage intentionally supports only one isolated process. It does not yet
 * implement a general frame allocator, page faults, or multi-process VM.
 */

#include <stddef.h>
#include <stdint.h>

#include "arch/x86/paging.h"
#include "kernel/process.h"
#include "kernel/vm.h"

#define VM_KERNEL_HIGH_TABLE_COUNT \
    (VM_KERNEL_HIGH_MAP_BYTES / (X86_PAGE_SIZE * X86_PAGE_TABLE_ENTRIES))
#define VM_FRAME_POOL_PAGES 16u
static uint32_t process_page_directories[2][X86_PAGE_DIRECTORY_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint32_t process_kernel_high_page_tables[2][VM_KERNEL_HIGH_TABLE_COUNT][X86_PAGE_TABLE_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint32_t process_user_text_page_tables[2][X86_PAGE_TABLE_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint32_t process_user_stack_page_tables[2][X86_PAGE_TABLE_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint8_t process_frame_pools[2][VM_FRAME_POOL_PAGES * X86_PAGE_SIZE]
    __attribute__((aligned(X86_PAGE_SIZE)));
static struct vm_runtime vm_runtimes[2];
static size_t active_vm_runtime_index;
static int vm_runtimes_initialized;

static uint32_t vm_kernel_pointer_to_physical(const void *pointer) {
    return (uint32_t)((uintptr_t)pointer - (uintptr_t)VM_KERNEL_VIRT_BASE);
}

static void vm_configure_runtime(struct vm_runtime *runtime, size_t runtime_index) {
    runtime->page_directory = &process_page_directories[runtime_index][0];
    runtime->kernel_high_page_tables = &process_kernel_high_page_tables[runtime_index][0][0];
    runtime->kernel_high_table_count = VM_KERNEL_HIGH_TABLE_COUNT;
    runtime->user_text_page_table = &process_user_text_page_tables[runtime_index][0];
    runtime->user_stack_page_table = &process_user_stack_page_tables[runtime_index][0];
    runtime->frame_pool = &process_frame_pools[runtime_index][0];
    runtime->frame_pool_page_count = VM_FRAME_POOL_PAGES;
    runtime->next_free_frame_page = 0;
    runtime->pointer_to_physical = vm_kernel_pointer_to_physical;
#if defined(__ELF__) && defined(__i386__)
    runtime->load_page_directory = paging_load_page_directory;
#else
    runtime->load_page_directory = NULL;
#endif
}

static void vm_initialize_runtimes(void) {
    if (vm_runtimes_initialized) {
        return;
    }

    vm_configure_runtime(&vm_runtimes[0], 0u);
    vm_configure_runtime(&vm_runtimes[1], 1u);
    active_vm_runtime_index = 0u;
    vm_runtimes_initialized = 1;
}

static void vm_zero_bytes(void *buffer, size_t byte_count) {
    uint8_t *bytes;
    size_t index;

    bytes = (uint8_t *)buffer;
    for (index = 0; index < byte_count; index++) {
        bytes[index] = 0;
    }
}

static void vm_copy_bytes(void *dst, const void *src, size_t byte_count) {
    uint8_t *dst_bytes;
    const uint8_t *src_bytes;
    size_t index;

    dst_bytes = (uint8_t *)dst;
    src_bytes = (const uint8_t *)src;
    for (index = 0; index < byte_count; index++) {
        dst_bytes[index] = src_bytes[index];
    }
}

static void *vm_alloc_pages_from_runtime(
    struct vm_runtime *runtime,
    size_t page_count
) {
    uint8_t *base;

    if (
        runtime == NULL ||
        runtime->frame_pool == NULL ||
        runtime->frame_pool_page_count == 0 ||
        runtime->next_free_frame_page + page_count > runtime->frame_pool_page_count
    ) {
        return NULL;
    }

    base = &runtime->frame_pool[runtime->next_free_frame_page * X86_PAGE_SIZE];
    runtime->next_free_frame_page += page_count;
    vm_zero_bytes(base, page_count * X86_PAGE_SIZE);
    return base;
}

struct vm_elf32_ehdr {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed));

struct vm_elf32_phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed));

#define VM_ELF_MAGIC0 0x7Fu
#define VM_ELF_MAGIC1 'E'
#define VM_ELF_MAGIC2 'L'
#define VM_ELF_MAGIC3 'F'
#define VM_ELFCLASS32 1u
#define VM_ELFDATA2LSB 1u
#define VM_ET_EXEC 2u
#define VM_EM_386 3u
#define VM_EV_CURRENT 1u
#define VM_PT_LOAD 1u

int vm_user_image_from_elf(
    struct vm_user_image *user_image,
    const uint8_t *elf_bytes,
    size_t elf_size
) {
    const struct vm_elf32_ehdr *ehdr;
    const struct vm_elf32_phdr *phdrs;
    const struct vm_elf32_phdr *load_phdr;
    uint16_t index;

    if (user_image == NULL || elf_bytes == NULL || elf_size < sizeof(*ehdr)) {
        return -1;
    }

    ehdr = (const struct vm_elf32_ehdr *)elf_bytes;
    if (
        ehdr->e_ident[0] != VM_ELF_MAGIC0 ||
        ehdr->e_ident[1] != VM_ELF_MAGIC1 ||
        ehdr->e_ident[2] != VM_ELF_MAGIC2 ||
        ehdr->e_ident[3] != VM_ELF_MAGIC3 ||
        ehdr->e_ident[4] != VM_ELFCLASS32 ||
        ehdr->e_ident[5] != VM_ELFDATA2LSB ||
        ehdr->e_type != VM_ET_EXEC ||
        ehdr->e_machine != VM_EM_386 ||
        ehdr->e_version != VM_EV_CURRENT ||
        ehdr->e_ehsize != sizeof(*ehdr) ||
        ehdr->e_phentsize != sizeof(struct vm_elf32_phdr)
    ) {
        return -1;
    }

    if (
        ehdr->e_phoff > elf_size ||
        (size_t)ehdr->e_phnum * sizeof(struct vm_elf32_phdr) > elf_size - ehdr->e_phoff
    ) {
        return -1;
    }

    phdrs = (const struct vm_elf32_phdr *)(elf_bytes + ehdr->e_phoff);
    load_phdr = NULL;
    for (index = 0; index < ehdr->e_phnum; index++) {
        if (phdrs[index].p_type != VM_PT_LOAD) {
            continue;
        }

        if (load_phdr != NULL) {
            return -1;
        }
        load_phdr = &phdrs[index];
    }

    if (
        load_phdr == NULL ||
        load_phdr->p_vaddr != USER_TEXT_BASE ||
        load_phdr->p_filesz > load_phdr->p_memsz ||
        load_phdr->p_offset > elf_size ||
        load_phdr->p_filesz > elf_size - load_phdr->p_offset ||
        ehdr->e_entry < load_phdr->p_vaddr ||
        ehdr->e_entry >= load_phdr->p_vaddr + load_phdr->p_memsz
    ) {
        return -1;
    }

    user_image->load_start = elf_bytes + load_phdr->p_offset;
    user_image->load_size = load_phdr->p_filesz;
    user_image->memory_size = load_phdr->p_memsz;
    user_image->entry_offset = ehdr->e_entry - load_phdr->p_vaddr;
    return 0;
}

size_t vm_page_count(size_t byte_count) {
    if (byte_count == 0) {
        return 0;
    }

    return (byte_count + X86_PAGE_SIZE - 1u) / X86_PAGE_SIZE;
}

uintptr_t vm_stack_guard_base(uintptr_t stack_top, size_t stack_size) {
    return stack_top - stack_size - X86_PAGE_SIZE;
}

void vm_space_init(struct vm_space *space, size_t user_text_size) {
    space->user_base = USER_BASE;
    space->user_limit = USER_LIMIT;
    space->text_base = USER_TEXT_BASE;
    space->text_size = user_text_size;
    space->stack_top = USER_STACK_TOP;
    space->stack_size = VM_USER_STACK_PAGES * X86_PAGE_SIZE;
    space->page_directory = NULL;
}

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
) {
    size_t index;
    size_t text_page_count;
    size_t stack_page_count;
    uint32_t kernel_flags;
    uint32_t user_flags;
    uint32_t kernel_high_base;
    uintptr_t stack_base;
    uint32_t stack_page_index;

    if (pointer_to_physical == NULL) {
        return;
    }

    kernel_flags = X86_PAGE_PRESENT | X86_PAGE_WRITABLE;
    user_flags = X86_PAGE_PRESENT | X86_PAGE_WRITABLE | X86_PAGE_USER;
    text_page_count = vm_page_count(space->text_size);
    stack_page_count = vm_page_count(space->stack_size);
    stack_base = space->stack_top - space->stack_size;
    kernel_high_base = VM_KERNEL_VIRT_BASE >> 22;

    for (index = 0; index < X86_PAGE_DIRECTORY_ENTRIES; index++) {
        page_directory[index] = 0;
    }

    for (index = 0; index < X86_PAGE_TABLE_ENTRIES; index++) {
        user_text_page_table[index] = 0;
        user_stack_page_table[index] = 0;
    }

    for (index = 0; index < kernel_high_table_count; index++) {
        size_t page_index;
        uint32_t *current_table;
        uint32_t physical_base;

        current_table =
            &kernel_high_page_tables[index * X86_PAGE_TABLE_ENTRIES];
        physical_base =
            (uint32_t)(index * X86_PAGE_TABLE_ENTRIES * X86_PAGE_SIZE);

        for (page_index = 0; page_index < X86_PAGE_TABLE_ENTRIES; page_index++) {
            current_table[page_index] =
                (physical_base + ((uint32_t)page_index * X86_PAGE_SIZE))
                | kernel_flags;
        }

        page_directory[kernel_high_base + index] =
            pointer_to_physical(current_table) | kernel_flags;
    }

    page_directory[space->text_base >> 22] =
        pointer_to_physical(user_text_page_table) | user_flags;
    for (index = 0; index < text_page_count; index++) {
        user_text_page_table[index] =
            (user_text_physical_base + ((uint32_t)index * X86_PAGE_SIZE))
            | user_flags;
    }

    page_directory[stack_base >> 22] =
        pointer_to_physical(user_stack_page_table) | user_flags;
    stack_page_index = (uint32_t)((stack_base >> 12) & 0x3FFu);
    for (index = 0; index < stack_page_count; index++) {
        user_stack_page_table[stack_page_index + index] =
            (user_stack_physical_base + ((uint32_t)index * X86_PAGE_SIZE))
            | user_flags;
    }
}

int vm_init_process(
    struct process *process,
    struct vm_runtime *runtime,
    const struct vm_user_image *user_image
) {
    uint8_t *user_text_backing;
    uint8_t *user_stack_backing;
    size_t user_text_page_count;

    if (
        process == NULL ||
        runtime == NULL ||
        user_image == NULL ||
        runtime->page_directory == NULL ||
        runtime->kernel_high_page_tables == NULL ||
        runtime->user_text_page_table == NULL ||
        runtime->user_stack_page_table == NULL ||
        runtime->pointer_to_physical == NULL ||
        user_image->load_start == NULL ||
        user_image->load_size > user_image->memory_size
    ) {
        return -1;
    }

    user_text_page_count = vm_page_count(user_image->memory_size);
    vm_space_init(&process->vm_space, user_image->memory_size);

    user_text_backing =
        (uint8_t *)vm_alloc_pages_from_runtime(runtime, user_text_page_count);
    user_stack_backing =
        (uint8_t *)vm_alloc_pages_from_runtime(runtime, VM_USER_STACK_PAGES);
    if (user_text_backing == NULL || user_stack_backing == NULL) {
        return -1;
    }

    vm_copy_bytes(user_text_backing, user_image->load_start, user_image->load_size);
    if (user_image->memory_size > user_image->load_size) {
        vm_zero_bytes(
            user_text_backing + user_image->load_size,
            user_image->memory_size - user_image->load_size
        );
    }

    vm_build_process_page_tables(
        runtime->page_directory,
        runtime->kernel_high_page_tables,
        runtime->user_text_page_table,
        runtime->user_stack_page_table,
        &process->vm_space,
        runtime->pointer_to_physical,
        runtime->kernel_high_table_count,
        runtime->pointer_to_physical(user_text_backing),
        runtime->pointer_to_physical(user_stack_backing)
    );

    process->vm_space.page_directory = runtime->page_directory;
    process->entry_point = USER_TEXT_BASE + user_image->entry_offset;
    process->user_stack_top = USER_STACK_TOP;

    return 0;
}

void vm_activate_process(
    const struct process *process,
    const struct vm_runtime *runtime
) {
    if (
        process != NULL &&
        runtime != NULL &&
        runtime->pointer_to_physical != NULL &&
        runtime->load_page_directory != NULL &&
        process->vm_space.page_directory != NULL
    ) {
        runtime->load_page_directory(
            (const void *)(uintptr_t)runtime->pointer_to_physical(
                process->vm_space.page_directory
            )
        );
    }
}

void vm_reset_runtime(struct vm_runtime *runtime) {
    if (runtime != NULL) {
        runtime->next_free_frame_page = 0;
    }
}

struct vm_runtime *vm_prepare_next_runtime(void) {
    size_t next_runtime_index;

    vm_initialize_runtimes();
    next_runtime_index = active_vm_runtime_index ^ 1u;
    vm_reset_runtime(&vm_runtimes[next_runtime_index]);
    return &vm_runtimes[next_runtime_index];
}

void vm_set_active_runtime(struct vm_runtime *runtime) {
    vm_initialize_runtimes();

    if (runtime == &vm_runtimes[0]) {
        active_vm_runtime_index = 0u;
    } else if (runtime == &vm_runtimes[1]) {
        active_vm_runtime_index = 1u;
    }
}

struct vm_runtime *vm_default_runtime(void) {
    vm_initialize_runtimes();
    return &vm_runtimes[active_vm_runtime_index];
}
