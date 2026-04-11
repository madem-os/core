#include <stdint.h>
#include <stddef.h>

#include "input/input.h"
#include "test.h"

struct scancode_expectation {
    uint8_t scancode;
    uint8_t expected;
};

static void test_expect_sequence(const uint8_t *expected, size_t count) {
    size_t i;
    uint8_t value = 0;

    for (i = 0; i < count; i++) {
        EXPECT_TRUE(input_read_byte(&value));
        EXPECT_EQ_U8(expected[i], value);
    }

    EXPECT_FALSE(input_has_byte());
}

static void test_init_starts_empty(void) {
    input_init();

    EXPECT_FALSE(input_has_byte());
}

static void test_letter_press_enqueues_lowercase_byte(void) {
    uint8_t value = 0;

    input_init();
    input_handle_scancode(0x1E);

    EXPECT_TRUE(input_has_byte());
    EXPECT_TRUE(input_read_byte(&value));
    EXPECT_EQ_U8((uint8_t)'a', value);
}

static void test_shift_and_letter_produce_uppercase_byte(void) {
    input_init();
    input_handle_scancode(0x2A);
    input_handle_scancode(0x1E);

    test_expect_sequence((const uint8_t[]){(uint8_t)'A'}, 1u);
}

static void test_shift_release_restores_lowercase_translation(void) {
    uint8_t value = 0;

    input_init();
    input_handle_scancode(0x2A);
    input_handle_scancode(0xAA);
    input_handle_scancode(0x1E);

    EXPECT_TRUE(input_read_byte(&value));
    EXPECT_EQ_U8((uint8_t)'a', value);
}

static void test_key_release_enqueues_nothing(void) {
    uint8_t value = 0;

    input_init();
    input_handle_scancode(0x9E);

    EXPECT_FALSE(input_has_byte());
    EXPECT_FALSE(input_read_byte(&value));
}

static void test_enter_enqueues_newline(void) {
    input_init();
    input_handle_scancode(0x1C);

    test_expect_sequence((const uint8_t[]){(uint8_t)'\n'}, 1u);
}

static void test_backspace_enqueues_backspace_byte(void) {
    input_init();
    input_handle_scancode(0x0E);

    test_expect_sequence((const uint8_t[]){(uint8_t)'\b'}, 1u);
}

static void test_unshifted_english_layout_bytes_are_translated(void) {
    static const struct scancode_expectation expectations[] = {
        {0x02, (uint8_t)'1'},
        {0x03, (uint8_t)'2'},
        {0x0C, (uint8_t)'-'},
        {0x0D, (uint8_t)'='},
        {0x0F, (uint8_t)'\t'},
        {0x10, (uint8_t)'q'},
        {0x1A, (uint8_t)'['},
        {0x1B, (uint8_t)']'},
        {0x27, (uint8_t)';'},
        {0x28, (uint8_t)'\''},
        {0x29, (uint8_t)'`'},
        {0x2B, (uint8_t)'\\'},
        {0x33, (uint8_t)','},
        {0x34, (uint8_t)'.'},
        {0x35, (uint8_t)'/'},
        {0x39, (uint8_t)' '},
    };
    size_t i;
    uint8_t value = 0;

    input_init();

    for (i = 0; i < sizeof(expectations) / sizeof(expectations[0]); i++) {
        input_handle_scancode(expectations[i].scancode);
        EXPECT_TRUE(input_read_byte(&value));
        EXPECT_EQ_U8(expectations[i].expected, value);
    }

    EXPECT_FALSE(input_has_byte());
}

static void test_shifted_number_row_produces_symbol_bytes(void) {
    static const struct scancode_expectation expectations[] = {
        {0x02, (uint8_t)'!'},
        {0x03, (uint8_t)'@'},
        {0x04, (uint8_t)'#'},
        {0x05, (uint8_t)'$'},
        {0x06, (uint8_t)'%'},
        {0x07, (uint8_t)'^'},
        {0x08, (uint8_t)'&'},
        {0x09, (uint8_t)'*'},
        {0x0A, (uint8_t)'('},
        {0x0B, (uint8_t)')'},
    };
    size_t i;
    uint8_t value = 0;

    input_init();

    for (i = 0; i < sizeof(expectations) / sizeof(expectations[0]); i++) {
        input_handle_scancode(0x2A);
        input_handle_scancode(expectations[i].scancode);
        input_handle_scancode(0xAA);
        EXPECT_TRUE(input_read_byte(&value));
        EXPECT_EQ_U8(expectations[i].expected, value);
    }

    EXPECT_FALSE(input_has_byte());
}

static void test_shifted_punctuation_produces_symbol_bytes(void) {
    static const struct scancode_expectation expectations[] = {
        {0x0C, (uint8_t)'_'},
        {0x0D, (uint8_t)'+'},
        {0x1A, (uint8_t)'{'},
        {0x1B, (uint8_t)'}'},
        {0x27, (uint8_t)':'},
        {0x28, (uint8_t)'"'},
        {0x29, (uint8_t)'~'},
        {0x2B, (uint8_t)'|'},
        {0x33, (uint8_t)'<'},
        {0x34, (uint8_t)'>'},
        {0x35, (uint8_t)'?'},
    };
    size_t i;
    uint8_t value = 0;

    input_init();

    for (i = 0; i < sizeof(expectations) / sizeof(expectations[0]); i++) {
        input_handle_scancode(0x2A);
        input_handle_scancode(expectations[i].scancode);
        input_handle_scancode(0xAA);
        EXPECT_TRUE(input_read_byte(&value));
        EXPECT_EQ_U8(expectations[i].expected, value);
    }

    EXPECT_FALSE(input_has_byte());
}

static void test_arrow_keys_expand_to_escape_sequences(void) {
    input_init();

    input_handle_scancode(0xE0);
    input_handle_scancode(0x48);
    test_expect_sequence((const uint8_t[]){0x1B, (uint8_t)'[', (uint8_t)'A'}, 3u);

    input_handle_scancode(0xE0);
    input_handle_scancode(0x50);
    test_expect_sequence((const uint8_t[]){0x1B, (uint8_t)'[', (uint8_t)'B'}, 3u);

    input_handle_scancode(0xE0);
    input_handle_scancode(0x4B);
    test_expect_sequence((const uint8_t[]){0x1B, (uint8_t)'[', (uint8_t)'D'}, 3u);

    input_handle_scancode(0xE0);
    input_handle_scancode(0x4D);
    test_expect_sequence((const uint8_t[]){0x1B, (uint8_t)'[', (uint8_t)'C'}, 3u);
}

static void test_blocking_read_returns_buffered_byte(void) {
    input_init();
    input_handle_scancode(0x1E);

    EXPECT_EQ_U8((uint8_t)'a', input_read_byte_blocking());
}

int main(void) {
    RUN_TEST(test_init_starts_empty);
    RUN_TEST(test_letter_press_enqueues_lowercase_byte);
    RUN_TEST(test_shift_and_letter_produce_uppercase_byte);
    RUN_TEST(test_shift_release_restores_lowercase_translation);
    RUN_TEST(test_key_release_enqueues_nothing);
    RUN_TEST(test_enter_enqueues_newline);
    RUN_TEST(test_backspace_enqueues_backspace_byte);
    RUN_TEST(test_unshifted_english_layout_bytes_are_translated);
    RUN_TEST(test_shifted_number_row_produces_symbol_bytes);
    RUN_TEST(test_shifted_punctuation_produces_symbol_bytes);
    RUN_TEST(test_arrow_keys_expand_to_escape_sequences);
    RUN_TEST(test_blocking_read_returns_buffered_byte);
    return test_failures_total == 0 ? 0 : 1;
}
