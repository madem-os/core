#include <stddef.h>
#include <stdint.h>

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
    char *output = (char *)ctx;
    int i;

    for (i = 0; i < len; i++) {
        output[test_output_len++] = buf[i];
    }

    return len;
}

static void test_set_input_bytes(const uint8_t *bytes, size_t count) {
    test_input_bytes = bytes;
    test_input_count = count;
    test_input_index = 0;
}

static void test_reset_output(void) {
    size_t i;

    test_output_len = 0;
    for (i = 0; i < sizeof(test_output_buffer); i++) {
        test_output_buffer[i] = 0;
    }
}

static void test_init_defaults_to_cooked_mode(void) {
    struct tty tty;

    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);

    EXPECT_TRUE(tty.mode == TTY_MODE_COOKED);
    EXPECT_TRUE(tty.echo_enabled);
    EXPECT_TRUE(tty.read_input_byte_blocking == test_read_input_byte_blocking);
    EXPECT_TRUE(tty.write_output == test_write_output);
    EXPECT_TRUE(tty.output_ctx == test_output_buffer);
    EXPECT_TRUE(tty_get_mode(&tty) == TTY_MODE_COOKED);
}

static void test_set_mode_switches_between_raw_and_cooked(void) {
    struct tty tty;

    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);

    tty_set_mode(&tty, TTY_MODE_RAW);
    EXPECT_TRUE(tty_get_mode(&tty) == TTY_MODE_RAW);

    tty_set_mode(&tty, TTY_MODE_COOKED);
    EXPECT_TRUE(tty_get_mode(&tty) == TTY_MODE_COOKED);
}

static void test_raw_mode_returns_next_byte_immediately(void) {
    static const uint8_t bytes[] = {(uint8_t)'a'};
    struct tty tty;

    test_set_input_bytes(bytes, sizeof(bytes));
    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);
    tty_set_mode(&tty, TTY_MODE_RAW);

    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)tty_read_raw_blocking(&tty));
}

static void test_cooked_mode_returns_completed_line_after_enter(void) {
    static const uint8_t bytes[] = {(uint8_t)'a', (uint8_t)'b', (uint8_t)'\n'};
    struct tty tty;
    char buf[8] = {0};
    int count;

    test_set_input_bytes(bytes, sizeof(bytes));
    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);

    count = tty_read(&tty, buf, (int)sizeof(buf));

    EXPECT_TRUE(count == 3);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)buf[0]);
    EXPECT_EQ_U8((uint8_t)'b', (uint8_t)buf[1]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)buf[2]);
    EXPECT_TRUE(test_output_len == 3);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)test_output_buffer[0]);
    EXPECT_EQ_U8((uint8_t)'b', (uint8_t)test_output_buffer[1]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)test_output_buffer[2]);
}

static void test_cooked_mode_handles_backspace(void) {
    static const uint8_t bytes[] = {
        (uint8_t)'a', (uint8_t)'b', (uint8_t)'\b', (uint8_t)'c', (uint8_t)'\n'
    };
    struct tty tty;
    char buf[8] = {0};
    int count;

    test_set_input_bytes(bytes, sizeof(bytes));
    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);

    count = tty_read(&tty, buf, (int)sizeof(buf));

    EXPECT_TRUE(count == 3);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)buf[0]);
    EXPECT_EQ_U8((uint8_t)'c', (uint8_t)buf[1]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)buf[2]);
    EXPECT_TRUE(test_output_len == 7);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)test_output_buffer[0]);
    EXPECT_EQ_U8((uint8_t)'b', (uint8_t)test_output_buffer[1]);
    EXPECT_EQ_U8((uint8_t)'\b', (uint8_t)test_output_buffer[2]);
    EXPECT_EQ_U8((uint8_t)' ', (uint8_t)test_output_buffer[3]);
    EXPECT_EQ_U8((uint8_t)'\b', (uint8_t)test_output_buffer[4]);
    EXPECT_EQ_U8((uint8_t)'c', (uint8_t)test_output_buffer[5]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)test_output_buffer[6]);
}

static void test_tty_read_uses_mode_specific_behavior(void) {
    static const uint8_t raw_bytes[] = {(uint8_t)'a'};
    static const uint8_t cooked_bytes[] = {(uint8_t)'a', (uint8_t)'\n'};
    struct tty tty;
    char buf[8] = {0};
    int count;

    test_set_input_bytes(raw_bytes, sizeof(raw_bytes));
    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);
    tty_set_mode(&tty, TTY_MODE_RAW);

    count = tty_read(&tty, buf, (int)sizeof(buf));
    EXPECT_TRUE(count == 1);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)buf[0]);
    EXPECT_TRUE(test_output_len == 0);

    test_set_input_bytes(cooked_bytes, sizeof(cooked_bytes));
    test_reset_output();
    tty_set_mode(&tty, TTY_MODE_COOKED);

    count = tty_read(&tty, buf, (int)sizeof(buf));
    EXPECT_TRUE(count == 2);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)buf[0]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)buf[1]);
    EXPECT_TRUE(test_output_len == 2);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)test_output_buffer[0]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)test_output_buffer[1]);
}

static void test_tty_write_forwards_bytes_to_output_backend(void) {
    struct tty tty;
    static const char bytes[] = "abc\n";

    test_reset_output();
    tty_init(&tty, test_read_input_byte_blocking, test_write_output, test_output_buffer);

    EXPECT_TRUE(tty_write(&tty, bytes, 4) == 4);
    EXPECT_TRUE(test_output_len == 4);
    EXPECT_EQ_U8((uint8_t)'a', (uint8_t)test_output_buffer[0]);
    EXPECT_EQ_U8((uint8_t)'b', (uint8_t)test_output_buffer[1]);
    EXPECT_EQ_U8((uint8_t)'c', (uint8_t)test_output_buffer[2]);
    EXPECT_EQ_U8((uint8_t)'\n', (uint8_t)test_output_buffer[3]);
}

int main(void) {
    RUN_TEST(test_init_defaults_to_cooked_mode);
    RUN_TEST(test_set_mode_switches_between_raw_and_cooked);
    RUN_TEST(test_raw_mode_returns_next_byte_immediately);
    RUN_TEST(test_cooked_mode_returns_completed_line_after_enter);
    RUN_TEST(test_cooked_mode_handles_backspace);
    RUN_TEST(test_tty_read_uses_mode_specific_behavior);
    RUN_TEST(test_tty_write_forwards_bytes_to_output_backend);
    return test_failures_total == 0 ? 0 : 1;
}
