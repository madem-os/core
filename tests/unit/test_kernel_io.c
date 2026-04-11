#include <stddef.h>
#include <stdint.h>

#include "kernel/io.h"
#include "kernel/process.h"
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

static void test_process_init_clears_fd_slots(void) {
    struct process process;

    process_init(&process);

    EXPECT_TRUE(process.fds[0].kind == FD_KIND_NONE);
    EXPECT_TRUE(process.fds[1].kind == FD_KIND_NONE);
    EXPECT_TRUE(process.fds[2].kind == FD_KIND_NONE);
    EXPECT_TRUE(process.fds[0].object == NULL);
    EXPECT_TRUE(process.fds[1].object == NULL);
    EXPECT_TRUE(process.fds[2].object == NULL);
    EXPECT_TRUE(process.entry_point == 0u);
    EXPECT_TRUE(process.user_stack_top == 0u);
    EXPECT_TRUE(process.vm_space.user_base == 0u);
    EXPECT_TRUE(process.vm_space.user_limit == 0u);
    EXPECT_TRUE(process.vm_space.text_base == 0u);
    EXPECT_TRUE(process.vm_space.text_size == 0u);
    EXPECT_TRUE(process.vm_space.stack_top == 0u);
    EXPECT_TRUE(process.vm_space.stack_size == 0u);
    EXPECT_TRUE(process.vm_space.page_directory == NULL);
}

static void test_process_set_tty_stdio_wires_all_three_stdio_slots(void) {
    struct process process;
    struct tty tty;

    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);
    process_init(&process);
    process_set_tty_stdio(&process, &tty);

    EXPECT_TRUE(process.fds[0].kind == FD_KIND_TTY);
    EXPECT_TRUE(process.fds[1].kind == FD_KIND_TTY);
    EXPECT_TRUE(process.fds[2].kind == FD_KIND_TTY);
    EXPECT_TRUE(process.fds[0].object == &tty);
    EXPECT_TRUE(process.fds[1].object == &tty);
    EXPECT_TRUE(process.fds[2].object == &tty);
}

static void test_kread_dispatches_stdin_to_tty(void) {
    static const uint8_t bytes[] = {(uint8_t)'a', (uint8_t)'\n'};
    struct process process;
    struct tty tty;
    char buf[8] = {0};
    int count;

    test_set_input_bytes(bytes, sizeof(bytes));
    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);
    process_init(&process);
    process_set_tty_stdio(&process, &tty);
    process_set_current(&process);

    count = kread(0, buf, (int)sizeof(buf));

    EXPECT_TRUE(count == 2);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)buf[0]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)buf[1]);
    EXPECT_TRUE(test_output_len == 2);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)test_output_buffer[0]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)test_output_buffer[1]);
}

static void test_kwrite_dispatches_stdout_and_stderr_to_tty(void) {
    struct process process;
    struct tty tty;
    static const char stdout_bytes[] = "abc";
    static const char stderr_bytes[] = "!\n";

    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);
    process_init(&process);
    process_set_tty_stdio(&process, &tty);
    process_set_current(&process);

    EXPECT_TRUE(kwrite(1, stdout_bytes, 3) == 3);
    EXPECT_TRUE(kwrite(2, stderr_bytes, 2) == 2);
    EXPECT_TRUE(test_output_len == 5);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)test_output_buffer[0]);
    EXPECT_EQ_U8((uint8_t)'b', (uint8_t)test_output_buffer[1]);
    EXPECT_EQ_U8((uint8_t)'c', (uint8_t)test_output_buffer[2]);
    EXPECT_EQ_U8((uint8_t)'!', (uint8_t)test_output_buffer[3]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)test_output_buffer[4]);
}

static void test_invalid_or_unsupported_fd_operations_fail(void) {
    struct process process;
    struct tty tty;
    char buf[8] = {0};

    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);
    process_init(&process);
    process_set_tty_stdio(&process, &tty);
    process_set_current(&process);

    EXPECT_TRUE(kread(1, buf, (int)sizeof(buf)) == -1);
    EXPECT_TRUE(kread(2, buf, (int)sizeof(buf)) == -1);
    EXPECT_TRUE(kwrite(0, "x", 1) == -1);
    EXPECT_TRUE(kread(3, buf, (int)sizeof(buf)) == -1);
    EXPECT_TRUE(kwrite(3, "x", 1) == -1);
    EXPECT_TRUE(kread(0, NULL, (int)sizeof(buf)) == -1);
    EXPECT_TRUE(kwrite(1, NULL, 1) == -1);
}

int main(void) {
    RUN_TEST(test_process_init_clears_fd_slots);
    RUN_TEST(test_process_set_tty_stdio_wires_all_three_stdio_slots);
    RUN_TEST(test_kread_dispatches_stdin_to_tty);
    RUN_TEST(test_kwrite_dispatches_stdout_and_stderr_to_tty);
    RUN_TEST(test_invalid_or_unsupported_fd_operations_fail);
    return test_failures_total == 0 ? 0 : 1;
}
