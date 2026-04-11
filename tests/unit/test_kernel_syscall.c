#include <stddef.h>
#include <stdint.h>

#include "kernel/process.h"
#include "kernel/syscall.h"
#include "test.h"
#include "tty/tty.h"

static const uint8_t *test_input_bytes;
static size_t test_input_count;
static size_t test_input_index;
static char test_output_buffer[64];
static size_t test_output_len;

static uint8_t test_read_input_byte_blocking(void) {
    if (test_input_index >= test_input_count) {
        return 0;
    }

    return test_input_bytes[test_input_index++];
}

static int test_write_output(void *ctx, const char *buf, int len) {
    char *output;
    int index;

    output = (char *)ctx;
    for (index = 0; index < len; index++) {
        output[test_output_len++] = buf[index];
    }

    return len;
}

static void test_set_input_bytes(const uint8_t *bytes, size_t count) {
    test_input_bytes = bytes;
    test_input_count = count;
    test_input_index = 0;
}

static void test_reset_output(void) {
    size_t index;

    test_output_len = 0;
    for (index = 0; index < sizeof(test_output_buffer); index++) {
        test_output_buffer[index] = 0;
    }
}

static void test_sys_read_dispatches_to_kread(void) {
    static const uint8_t bytes[] = {(uint8_t)'x', (uint8_t)'\n'};
    struct process process;
    struct tty tty;
    char buf[8] = {0};
    int result;

    test_set_input_bytes(bytes, sizeof(bytes));
    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);
    process_init(&process);
    process_set_tty_stdio(&process, &tty);
    process_set_current(&process);

    result = syscall_dispatch(
        SYS_READ,
        0,
        (kernel_syscall_arg_t)(uintptr_t)buf,
        (kernel_syscall_arg_t)sizeof(buf)
    );

    EXPECT_TRUE(result == 2);
    EXPECT_EQ_U8((uint8_t)'x', (uint8_t)buf[0]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)buf[1]);
}

static void test_sys_write_dispatches_to_kwrite(void) {
    struct process process;
    struct tty tty;
    static const char bytes[] = "ok";
    int result;

    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);
    process_init(&process);
    process_set_tty_stdio(&process, &tty);
    process_set_current(&process);

    result = syscall_dispatch(
        SYS_WRITE,
        1,
        (kernel_syscall_arg_t)(uintptr_t)bytes,
        2
    );

    EXPECT_TRUE(result == 2);
    EXPECT_TRUE(test_output_len == 2);
    EXPECT_EQ_U8((uint8_t)'o', (uint8_t)test_output_buffer[0]);
    EXPECT_EQ_U8((uint8_t)'k', (uint8_t)test_output_buffer[1]);
}

static void test_unsupported_syscall_returns_minus_one(void) {
    EXPECT_TRUE(syscall_dispatch(999, 0, 0, 0) == -1);
}

int main(void) {
    RUN_TEST(test_sys_read_dispatches_to_kread);
    RUN_TEST(test_sys_write_dispatches_to_kwrite);
    RUN_TEST(test_unsupported_syscall_returns_minus_one);
    return test_failures_total == 0 ? 0 : 1;
}
