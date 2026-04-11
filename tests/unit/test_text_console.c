#include <stdint.h>

#include "console/display.h"
#include "console/text_console.h"
#include "test.h"

#define TEST_DISPLAY_WIDTH 4u
#define TEST_DISPLAY_HEIGHT 2u
#define TEST_DISPLAY_SIZE (TEST_DISPLAY_WIDTH * TEST_DISPLAY_HEIGHT)
#define TEST_CELL_CHAR(cell) ((uint8_t)((cell) & 0x00FFu))

static uint16_t test_cells[TEST_DISPLAY_SIZE];
static unsigned int test_cursor_call_count;
static uint16_t test_cursor_row;
static uint16_t test_cursor_column;

static void test_write_cell(
    struct display *display,
    uint16_t row,
    uint16_t column,
    uint8_t ch,
    uint8_t color
) {
    uint16_t *cells = (uint16_t *)display->ctx;
    uint16_t offset = (uint16_t)(row * display->width + column);

    cells[offset] = (uint16_t)ch | ((uint16_t)color << 8);
}

static void test_set_cursor(struct display *display, uint16_t row, uint16_t column) {
    (void)display;
    test_cursor_call_count++;
    test_cursor_row = row;
    test_cursor_column = column;
}

static void test_reset_display_state(void) {
    uint16_t i;

    for (i = 0; i < TEST_DISPLAY_SIZE; i++) {
        test_cells[i] = 0;
    }
    test_cursor_call_count = 0;
    test_cursor_row = 0;
    test_cursor_column = 0;
}

static void test_init_sets_display_and_resets_cursor(void) {
    struct display display;
    struct text_console console;

    test_reset_display_state();
    display_init(
        &display,
        test_cells,
        TEST_DISPLAY_WIDTH,
        TEST_DISPLAY_HEIGHT,
        test_cells,
        test_write_cell,
        test_set_cursor
    );

    text_console_init(&console, &display);

    EXPECT_TRUE(console.display == &display);
    EXPECT_TRUE(console.row == 0u);
    EXPECT_TRUE(console.column == 0u);
}

static void test_write_printable_bytes_advances_cursor(void) {
    struct display display;
    struct text_console console;

    test_reset_display_state();
    display_init(
        &display,
        test_cells,
        TEST_DISPLAY_WIDTH,
        TEST_DISPLAY_HEIGHT,
        test_cells,
        test_write_cell,
        test_set_cursor
    );
    text_console_init(&console, &display);

    EXPECT_TRUE(text_console_write(&console, "ab", 2) == 2);
    EXPECT_EQ_U8((uint8_t)'a', TEST_CELL_CHAR(test_cells[0]));
    EXPECT_EQ_U8((uint8_t)'b', TEST_CELL_CHAR(test_cells[1]));
    EXPECT_TRUE(console.row == 0u);
    EXPECT_TRUE(console.column == 2u);
}

static void test_newline_moves_to_next_row_start(void) {
    struct display display;
    struct text_console console;

    test_reset_display_state();
    display_init(
        &display,
        test_cells,
        TEST_DISPLAY_WIDTH,
        TEST_DISPLAY_HEIGHT,
        test_cells,
        test_write_cell,
        test_set_cursor
    );
    text_console_init(&console, &display);

    EXPECT_TRUE(text_console_write(&console, "a\nb", 3) == 3);
    EXPECT_EQ_U8((uint8_t)'a', TEST_CELL_CHAR(test_cells[0]));
    EXPECT_EQ_U8((uint8_t)'b', TEST_CELL_CHAR(test_cells[4]));
    EXPECT_TRUE(console.row == 1u);
    EXPECT_TRUE(console.column == 1u);
}

static void test_backspace_erases_previous_cell(void) {
    struct display display;
    struct text_console console;

    test_reset_display_state();
    display_init(
        &display,
        test_cells,
        TEST_DISPLAY_WIDTH,
        TEST_DISPLAY_HEIGHT,
        test_cells,
        test_write_cell,
        test_set_cursor
    );
    text_console_init(&console, &display);

    EXPECT_TRUE(text_console_write(&console, "ab\b", 3) == 3);
    EXPECT_EQ_U8((uint8_t)'a', TEST_CELL_CHAR(test_cells[0]));
    EXPECT_EQ_U8((uint8_t)' ', TEST_CELL_CHAR(test_cells[1]));
    EXPECT_TRUE(console.row == 0u);
    EXPECT_TRUE(console.column == 1u);
}

static void test_wrap_and_scroll_keep_latest_visible_rows(void) {
    struct display display;
    struct text_console console;

    test_reset_display_state();
    display_init(
        &display,
        test_cells,
        TEST_DISPLAY_WIDTH,
        TEST_DISPLAY_HEIGHT,
        test_cells,
        test_write_cell,
        test_set_cursor
    );
    text_console_init(&console, &display);

    EXPECT_TRUE(text_console_write(&console, "abcd\nefgh\nij", 12) == 12);

    EXPECT_EQ_U8((uint8_t)'e', TEST_CELL_CHAR(test_cells[0]));
    EXPECT_EQ_U8((uint8_t)'f', TEST_CELL_CHAR(test_cells[1]));
    EXPECT_EQ_U8((uint8_t)'g', TEST_CELL_CHAR(test_cells[2]));
    EXPECT_EQ_U8((uint8_t)'h', TEST_CELL_CHAR(test_cells[3]));
    EXPECT_EQ_U8((uint8_t)'i', TEST_CELL_CHAR(test_cells[4]));
    EXPECT_EQ_U8((uint8_t)'j', TEST_CELL_CHAR(test_cells[5]));
    EXPECT_TRUE(console.row == 1u);
    EXPECT_TRUE(console.column == 2u);
}

static void test_text_console_tty_write_uses_console_context(void) {
    struct display display;
    struct text_console console;

    test_reset_display_state();
    display_init(
        &display,
        test_cells,
        TEST_DISPLAY_WIDTH,
        TEST_DISPLAY_HEIGHT,
        test_cells,
        test_write_cell,
        test_set_cursor
    );
    text_console_init(&console, &display);

    EXPECT_TRUE(text_console_tty_write(&console, "xy", 2) == 2);
    EXPECT_EQ_U8((uint8_t)'x', TEST_CELL_CHAR(test_cells[0]));
    EXPECT_EQ_U8((uint8_t)'y', TEST_CELL_CHAR(test_cells[1]));
}

int main(void) {
    RUN_TEST(test_init_sets_display_and_resets_cursor);
    RUN_TEST(test_write_printable_bytes_advances_cursor);
    RUN_TEST(test_newline_moves_to_next_row_start);
    RUN_TEST(test_backspace_erases_previous_cell);
    RUN_TEST(test_wrap_and_scroll_keep_latest_visible_rows);
    RUN_TEST(test_text_console_tty_write_uses_console_context);
    return test_failures_total == 0 ? 0 : 1;
}
