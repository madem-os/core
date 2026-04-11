#include <stdint.h>

#include "kernel/process.h"
#include "kernel/vm.h"
#include "test.h"

static uint32_t test_last_loaded_page_directory;

static uint32_t test_identity_pointer_to_physical(const void *pointer) {
    return (uint32_t)(uintptr_t)pointer;
}

static void test_record_load_page_directory(const void *page_directory_physical) {
    test_last_loaded_page_directory = (uint32_t)(uintptr_t)page_directory_physical;
}

static void test_vm_space_init_sets_fixed_user_contract(void) {
    struct vm_space space;

    vm_space_init(&space, 6000u);

    EXPECT_EQ_U32(USER_BASE, space.user_base);
    EXPECT_EQ_U32(USER_LIMIT, space.user_limit);
    EXPECT_EQ_U32(USER_TEXT_BASE, space.text_base);
    EXPECT_EQ_U32(6000u, (uint32_t)space.text_size);
    EXPECT_EQ_U32(USER_STACK_TOP, space.stack_top);
    EXPECT_EQ_U32(X86_PAGE_SIZE, (uint32_t)space.stack_size);
    EXPECT_TRUE(space.page_directory == NULL);
}

static void test_vm_page_count_rounds_up_by_page_size(void) {
    EXPECT_EQ_U32(0u, (uint32_t)vm_page_count(0u));
    EXPECT_EQ_U32(1u, (uint32_t)vm_page_count(1u));
    EXPECT_EQ_U32(1u, (uint32_t)vm_page_count(X86_PAGE_SIZE));
    EXPECT_EQ_U32(2u, (uint32_t)vm_page_count(X86_PAGE_SIZE + 1u));
}

static void test_vm_stack_guard_base_reserves_one_guard_page(void) {
    EXPECT_EQ_U32(
        USER_STACK_TOP - (2u * X86_PAGE_SIZE),
        (uint32_t)vm_stack_guard_base(USER_STACK_TOP, X86_PAGE_SIZE)
    );
}

static void test_vm_build_process_page_tables_sets_kernel_and_user_regions(void) {
    uint32_t page_directory[X86_PAGE_DIRECTORY_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    uint32_t kernel_low_page_table[X86_PAGE_TABLE_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    uint32_t kernel_high_page_tables[4][X86_PAGE_TABLE_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    uint32_t user_text_page_table[X86_PAGE_TABLE_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    uint32_t user_stack_page_table[X86_PAGE_TABLE_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    struct vm_space space;
    uint32_t expected_kernel_flags;
    uint32_t expected_user_flags;
    uint32_t stack_base;

    vm_space_init(&space, 5000u);
    expected_kernel_flags = X86_PAGE_PRESENT | X86_PAGE_WRITABLE;
    expected_user_flags =
        X86_PAGE_PRESENT | X86_PAGE_WRITABLE | X86_PAGE_USER;
    stack_base = (uint32_t)(space.stack_top - space.stack_size);

    vm_build_process_page_tables(
        page_directory,
        kernel_low_page_table,
        &kernel_high_page_tables[0][0],
        user_text_page_table,
        user_stack_page_table,
        &space,
        test_identity_pointer_to_physical,
        4u,
        0x00300000u,
        0x00302000u
    );

    EXPECT_EQ_U32(expected_kernel_flags, page_directory[0] & 0xFFFu);
    EXPECT_EQ_U32(expected_kernel_flags, page_directory[VM_KERNEL_VIRT_BASE >> 22] & 0xFFFu);
    EXPECT_EQ_U32(expected_kernel_flags, kernel_low_page_table[0] & 0xFFFu);
    EXPECT_EQ_U32(0x00000000u | expected_kernel_flags, kernel_low_page_table[0]);
    EXPECT_EQ_U32(0x00001000u | expected_kernel_flags, kernel_low_page_table[1]);

    EXPECT_EQ_U32(expected_user_flags, page_directory[USER_TEXT_BASE >> 22] & 0xFFFu);
    EXPECT_EQ_U32(0x00300000u | expected_user_flags, user_text_page_table[0]);
    EXPECT_EQ_U32(0x00301000u | expected_user_flags, user_text_page_table[1]);

    EXPECT_EQ_U32(expected_user_flags, page_directory[stack_base >> 22] & 0xFFFu);
    EXPECT_EQ_U32(0u, user_stack_page_table[((stack_base - X86_PAGE_SIZE) >> 12) & 0x3FFu]);
    EXPECT_EQ_U32(
        0x00302000u | expected_user_flags,
        user_stack_page_table[(stack_base >> 12) & 0x3FFu]
    );
}

static void test_vm_init_process_copies_user_image_and_sets_entry(void) {
    static const uint8_t image_bytes[] = {'A', 'B', 'C', 'D'};
    uint32_t page_directory[X86_PAGE_DIRECTORY_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    uint32_t kernel_low_page_table[X86_PAGE_TABLE_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    uint32_t kernel_high_page_tables[4][X86_PAGE_TABLE_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    uint32_t user_text_page_table[X86_PAGE_TABLE_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    uint32_t user_stack_page_table[X86_PAGE_TABLE_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    uint8_t frame_pool[4 * X86_PAGE_SIZE] __attribute__((aligned(X86_PAGE_SIZE)));
    struct vm_runtime runtime = {
        page_directory,
        kernel_low_page_table,
        &kernel_high_page_tables[0][0],
        4u,
        user_text_page_table,
        user_stack_page_table,
        frame_pool,
        4u,
        0u,
        test_identity_pointer_to_physical,
        test_record_load_page_directory
    };
    struct vm_user_image image = {
        image_bytes,
        sizeof(image_bytes),
        X86_PAGE_SIZE,
        0x24u
    };
    struct process process;

    process_init(&process);
    EXPECT_TRUE(vm_init_process(&process, &runtime, &image) == 0);
    EXPECT_EQ_U32(USER_TEXT_BASE + 0x24u, (uint32_t)process.entry_point);
    EXPECT_EQ_U32(USER_STACK_TOP, (uint32_t)process.user_stack_top);
    EXPECT_TRUE(process.vm_space.page_directory == page_directory);
    EXPECT_EQ_U8((uint8_t)'A', frame_pool[0]);
    EXPECT_EQ_U8((uint8_t)'D', frame_pool[3]);
    EXPECT_EQ_U8(0u, frame_pool[4]);
    EXPECT_EQ_U32(2u, (uint32_t)runtime.next_free_frame_page);
}

static void test_vm_activate_process_loads_physical_page_directory(void) {
    static uint32_t page_directory[X86_PAGE_DIRECTORY_ENTRIES]
        __attribute__((aligned(X86_PAGE_SIZE)));
    struct vm_runtime runtime = {
        page_directory,
        NULL,
        NULL,
        0u,
        NULL,
        NULL,
        NULL,
        0u,
        0u,
        test_identity_pointer_to_physical,
        test_record_load_page_directory
    };
    struct process process;

    process_init(&process);
    process.vm_space.page_directory = page_directory;
    test_last_loaded_page_directory = 0u;

    vm_activate_process(&process, &runtime);

    EXPECT_EQ_U32((uint32_t)(uintptr_t)page_directory, test_last_loaded_page_directory);
}

int main(void) {
    RUN_TEST(test_vm_space_init_sets_fixed_user_contract);
    RUN_TEST(test_vm_page_count_rounds_up_by_page_size);
    RUN_TEST(test_vm_stack_guard_base_reserves_one_guard_page);
    RUN_TEST(test_vm_build_process_page_tables_sets_kernel_and_user_regions);
    RUN_TEST(test_vm_init_process_copies_user_image_and_sets_entry);
    RUN_TEST(test_vm_activate_process_loads_physical_page_directory);
    return test_failures_total == 0 ? 0 : 1;
}
