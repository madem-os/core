#ifndef TESTS_UNIT_TEST_H
#define TESTS_UNIT_TEST_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern int test_current_failed;
extern int test_failures_total;

void test_begin(const char *name);
int test_finish(void);

static inline void test_fail_bool(const char *expr, const char *file, int line) {
    fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
    test_current_failed = 1;
    test_failures_total++;
}

static inline void test_fail_u8(
    const char *expr,
    uint8_t expected,
    uint8_t actual,
    const char *file,
    int line
) {
    fprintf(
        stderr,
        "%s:%d: assertion failed: %s (expected=%u actual=%u)\n",
        file,
        line,
        expr,
        (unsigned int)expected,
        (unsigned int)actual
    );
    test_current_failed = 1;
    test_failures_total++;
}

#define EXPECT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            test_fail_bool(#expr, __FILE__, __LINE__); \
        } \
    } while (0)

#define EXPECT_FALSE(expr) \
    do { \
        if ((expr)) { \
            test_fail_bool("!(" #expr ")", __FILE__, __LINE__); \
        } \
    } while (0)

#define EXPECT_EQ_U8(expected, actual) \
    do { \
        uint8_t test_expected_value = (uint8_t)(expected); \
        uint8_t test_actual_value = (uint8_t)(actual); \
        if (test_expected_value != test_actual_value) { \
            test_fail_u8( \
                #actual, \
                test_expected_value, \
                test_actual_value, \
                __FILE__, \
                __LINE__ \
            ); \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        test_begin(#fn); \
        fn(); \
        test_finish(); \
    } while (0)

#endif
