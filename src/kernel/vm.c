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
static uint32_t process_page_directory[X86_PAGE_DIRECTORY_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint32_t process_kernel_low_page_table[X86_PAGE_TABLE_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint32_t process_kernel_high_page_tables[VM_KERNEL_HIGH_TABLE_COUNT][X86_PAGE_TABLE_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint32_t process_user_text_page_table[X86_PAGE_TABLE_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint32_t process_user_stack_page_table[X86_PAGE_TABLE_ENTRIES]
    __attribute__((aligned(X86_PAGE_SIZE)));
static uint8_t process_frame_pool[VM_FRAME_POOL_PAGES * X86_PAGE_SIZE]
    __attribute__((aligned(X86_PAGE_SIZE)));
static struct vm_runtime default_vm_runtime = {
    process_page_directory,
    process_kernel_low_page_table,
    &process_kernel_high_page_tables[0][0],
    VM_KERNEL_HIGH_TABLE_COUNT,
    process_user_text_page_table,
    process_user_stack_page_table,
    process_frame_pool,
    VM_FRAME_POOL_PAGES,
    0,
    NULL,
    NULL
};

static uint32_t vm_kernel_pointer_to_physical(const void *pointer) {
    return (uint32_t)((uintptr_t)pointer - (uintptr_t)VM_KERNEL_VIRT_BASE);
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

#if defined(__ELF__) && defined(__i386__)
extern uint8_t _user_image_load_start[];
extern uint8_t _user_image_load_end[];
extern uint8_t _user_image_start[];
extern uint8_t _user_image_end[];
extern uint8_t _user_image_entry[];
static struct vm_user_image default_user_image;
#endif

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
    uint32_t *kernel_low_page_table,
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
        kernel_low_page_table[index] =
            ((uint32_t)index * X86_PAGE_SIZE) | kernel_flags;
        user_text_page_table[index] = 0;
        user_stack_page_table[index] = 0;
    }

    page_directory[0] =
        pointer_to_physical(kernel_low_page_table) | kernel_flags;

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
        runtime->kernel_low_page_table == NULL ||
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
        runtime->kernel_low_page_table,
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

struct vm_runtime *vm_default_runtime(void) {
    default_vm_runtime.pointer_to_physical = vm_kernel_pointer_to_physical;
#if defined(__ELF__) && defined(__i386__)
    default_vm_runtime.load_page_directory = paging_load_page_directory;
#else
    default_vm_runtime.load_page_directory = NULL;
#endif
    return &default_vm_runtime;
}

const struct vm_user_image *vm_default_user_image(void) {
#if defined(__ELF__) && defined(__i386__)
    default_user_image.load_start = _user_image_load_start;
    default_user_image.load_size =
        (size_t)(_user_image_load_end - _user_image_load_start);
    default_user_image.memory_size =
        (size_t)(_user_image_end - _user_image_start);
    default_user_image.entry_offset =
        (uintptr_t)(_user_image_entry - _user_image_start);
    return &default_user_image;
#else
    return NULL;
#endif
}
