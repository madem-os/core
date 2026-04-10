#include <stdint.h>

#include "drivers/keyboard_ring.h"
#include "test.h"

static void test_init_starts_empty(void) {
    struct keyboard_ring ring;

    keyboard_ring_init(&ring);

    EXPECT_TRUE(keyboard_ring_is_empty(&ring));
    EXPECT_FALSE(keyboard_ring_is_full(&ring));
}

static void test_push_then_pop_returns_same_byte(void) {
    struct keyboard_ring ring;
    uint8_t value = 0;

    keyboard_ring_init(&ring);

    EXPECT_TRUE(keyboard_ring_push(&ring, 0x1E));
    EXPECT_FALSE(keyboard_ring_is_empty(&ring));
    EXPECT_TRUE(keyboard_ring_pop(&ring, &value));
    EXPECT_EQ_U8(0x1E, value);
    EXPECT_TRUE(keyboard_ring_is_empty(&ring));
}

static void test_fill_reaches_full_and_rejects_next_push(void) {
    struct keyboard_ring ring;
    size_t i;

    keyboard_ring_init(&ring);

    for (i = 0; i < KEYBOARD_RING_CAPACITY; i++) {
        EXPECT_TRUE(keyboard_ring_push(&ring, (uint8_t)i));
    }

    EXPECT_TRUE(keyboard_ring_is_full(&ring));
    EXPECT_FALSE(keyboard_ring_push(&ring, 0xAA));
}

static void test_full_capacity_preserves_last_slot_data(void) {
    struct keyboard_ring ring;
    uint8_t value = 0;
    size_t i;

    keyboard_ring_init(&ring);

    for (i = 0; i < KEYBOARD_RING_CAPACITY; i++) {
        EXPECT_TRUE(keyboard_ring_push(&ring, (uint8_t)i));
    }

    for (i = 0; i < KEYBOARD_RING_CAPACITY; i++) {
        EXPECT_TRUE(keyboard_ring_pop(&ring, &value));
        EXPECT_EQ_U8((uint8_t)i, value);
    }

    EXPECT_TRUE(keyboard_ring_is_empty(&ring));
}

static void test_wraparound_preserves_fifo_order(void) {
    struct keyboard_ring ring;
    uint8_t value = 0;
    size_t i;

    keyboard_ring_init(&ring);

    for (i = 0; i < KEYBOARD_RING_CAPACITY; i++) {
        EXPECT_TRUE(keyboard_ring_push(&ring, (uint8_t)i));
    }

    for (i = 0; i < 8u; i++) {
        EXPECT_TRUE(keyboard_ring_pop(&ring, &value));
        EXPECT_EQ_U8((uint8_t)i, value);
    }

    for (i = 0; i < 8u; i++) {
        EXPECT_TRUE(keyboard_ring_push(&ring, (uint8_t)(0x80u + i)));
    }

    for (i = 8u; i < KEYBOARD_RING_CAPACITY; i++) {
        EXPECT_TRUE(keyboard_ring_pop(&ring, &value));
        EXPECT_EQ_U8((uint8_t)i, value);
    }

    for (i = 0; i < 8u; i++) {
        EXPECT_TRUE(keyboard_ring_pop(&ring, &value));
        EXPECT_EQ_U8((uint8_t)(0x80u + i), value);
    }

    EXPECT_TRUE(keyboard_ring_is_empty(&ring));
}

int main(void) {
    RUN_TEST(test_init_starts_empty);
    RUN_TEST(test_push_then_pop_returns_same_byte);
    RUN_TEST(test_fill_reaches_full_and_rejects_next_push);
    RUN_TEST(test_full_capacity_preserves_last_slot_data);
    RUN_TEST(test_wraparound_preserves_fifo_order);
    return test_failures_total == 0 ? 0 : 1;
}
