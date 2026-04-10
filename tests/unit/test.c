#include "test.h"

int test_current_failed;
int test_failures_total;

static const char *test_current_name;

void test_begin(const char *name) {
    test_current_name = name;
    test_current_failed = 0;
}

int test_finish(void) {
    if (test_current_failed != 0) {
        printf("[FAILED] %s\n", test_current_name);
        return 1;
    }

    printf("[PASSED] %s\n", test_current_name);
    return 0;
}
