#include <stddef.h>
#include <stdint.h>

#include "kernel/user_programs.h"
#include "test.h"

static const uint8_t test_app_a_bytes[] = {0x01u};
static const uint8_t test_app_b_bytes[] = {0x02u};

const struct user_program builtin_user_programs[] = {
    {"alpha", test_app_a_bytes, test_app_a_bytes + sizeof(test_app_a_bytes)},
    {"beta", test_app_b_bytes, test_app_b_bytes + sizeof(test_app_b_bytes)}
};

const size_t builtin_user_program_count =
    sizeof(builtin_user_programs) / sizeof(builtin_user_programs[0]);

static void test_user_program_find_returns_named_program(void) {
    const struct user_program *program;

    program = user_program_find("beta");
    EXPECT_TRUE(program != NULL);
    EXPECT_TRUE(program == &builtin_user_programs[1]);
    EXPECT_TRUE(program->elf_start == test_app_b_bytes);
}

static void test_user_program_find_returns_null_for_unknown_name(void) {
    EXPECT_TRUE(user_program_find("missing") == NULL);
}

int main(void) {
    RUN_TEST(test_user_program_find_returns_named_program);
    RUN_TEST(test_user_program_find_returns_null_for_unknown_name);
    return test_failures_total == 0 ? 0 : 1;
}
