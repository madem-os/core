#include <stdint.h>

#include "kernel/bootstrap_paging.h"
#include "test.h"

static void test_build_identity_clears_unused_directory_entries(void) {
    uint32_t page_directory[X86_PAGE_DIRECTORY_ENTRIES];
    uint32_t page_tables[BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT * X86_PAGE_TABLE_ENTRIES];
    uint32_t fake_page_table_base;
    uint32_t index;

    for (index = 0; index < X86_PAGE_DIRECTORY_ENTRIES; index++) {
        page_directory[index] = 0xFFFFFFFFu;
    }

    fake_page_table_base = 0x00200000u;

    bootstrap_paging_build_identity(
        page_directory,
        page_tables,
        BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT,
        fake_page_table_base,
        0,
        X86_PAGE_PRESENT | X86_PAGE_WRITABLE | X86_PAGE_USER
    );

    for (index = BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT; index < X86_PAGE_DIRECTORY_ENTRIES; index++) {
        EXPECT_EQ_U32(0, page_directory[index]);
    }
}

static void test_build_identity_populates_pde_addresses_and_flags(void) {
    uint32_t page_directory[X86_PAGE_DIRECTORY_ENTRIES];
    uint32_t page_tables[BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT * X86_PAGE_TABLE_ENTRIES];
    uint32_t fake_page_table_base;

    fake_page_table_base = 0x00200000u;

    bootstrap_paging_build_identity(
        page_directory,
        page_tables,
        BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT,
        fake_page_table_base,
        0,
        X86_PAGE_PRESENT | X86_PAGE_WRITABLE | X86_PAGE_USER
    );

    EXPECT_EQ_U32(
        fake_page_table_base | X86_PAGE_PRESENT | X86_PAGE_WRITABLE | X86_PAGE_USER,
        page_directory[0]
    );
    EXPECT_EQ_U32(
        (fake_page_table_base + X86_PAGE_SIZE) | X86_PAGE_PRESENT | X86_PAGE_WRITABLE | X86_PAGE_USER,
        page_directory[1]
    );
    EXPECT_EQ_U32(
        (fake_page_table_base + ((BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT - 1) * X86_PAGE_SIZE)) |
            X86_PAGE_PRESENT |
            X86_PAGE_WRITABLE |
            X86_PAGE_USER,
        page_directory[BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT - 1]
    );
}

static void test_build_identity_populates_contiguous_identity_ptes(void) {
    uint32_t page_directory[X86_PAGE_DIRECTORY_ENTRIES];
    uint32_t page_tables[BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT * X86_PAGE_TABLE_ENTRIES];

    bootstrap_paging_build_identity(
        page_directory,
        page_tables,
        BOOTSTRAP_IDENTITY_MAP_TABLE_COUNT,
        0x00200000u,
        0,
        X86_PAGE_PRESENT | X86_PAGE_WRITABLE | X86_PAGE_USER
    );

    EXPECT_EQ_U32(0x00000007u, page_tables[0]);
    EXPECT_EQ_U32(0x00001007u, page_tables[1]);
    EXPECT_EQ_U32(0x003FF007u, page_tables[X86_PAGE_TABLE_ENTRIES - 1]);
    EXPECT_EQ_U32(0x00400007u, page_tables[X86_PAGE_TABLE_ENTRIES]);
}

static void test_build_identity_supports_nonzero_start_physical_address(void) {
    uint32_t page_directory[X86_PAGE_DIRECTORY_ENTRIES];
    uint32_t page_tables[X86_PAGE_TABLE_ENTRIES];

    bootstrap_paging_build_identity(
        page_directory,
        page_tables,
        1,
        0x00300000u,
        0x00100000u,
        X86_PAGE_PRESENT | X86_PAGE_WRITABLE
    );

    EXPECT_EQ_U32(0x00100003u, page_tables[0]);
    EXPECT_EQ_U32(0x00101003u, page_tables[1]);
    EXPECT_EQ_U32(0x00300003u, page_directory[0]);
}

int main(void) {
    RUN_TEST(test_build_identity_clears_unused_directory_entries);
    RUN_TEST(test_build_identity_populates_pde_addresses_and_flags);
    RUN_TEST(test_build_identity_populates_contiguous_identity_ptes);
    RUN_TEST(test_build_identity_supports_nonzero_start_physical_address);
    return test_failures_total == 0 ? 0 : 1;
}
