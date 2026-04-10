# Input System Design

## Scope

This document defines the new input-system layer that sits between:

- the keyboard driver, which receives raw scancodes from hardware
- the TTY layer, which consumes terminal input bytes

This input-system task is the next implementation step after the design change
that moved translation and buffering out of the keyboard driver.

It is intentionally scoped to:

- scancode intake
- translation into terminal input bytes
- byte buffering
- blocking and non-blocking byte reads

It does not include:

- TTY raw/cooked behavior
- `read(fd, ...)`
- process or file descriptor state
- shell logic
- user mode

## Goal

Introduce a dedicated input module that:

1. accepts raw keyboard scancodes
2. tracks modifier state such as Shift
3. translates scancodes into terminal input bytes
4. stores those bytes in a ring buffer
5. exposes a small API for reading buffered bytes

This makes the architecture:

`keyboard driver -> input system -> tty -> read(fd, ...)`

instead of:

`keyboard driver -> tty`

or:

`keyboard driver -> console`

## Why This Layer Exists

The keyboard driver should be responsible for hardware interaction only.

If translation and buffering stay inside the keyboard driver, then the driver
also ends up owning:

- modifier semantics
- terminal-facing byte stream semantics
- policy about how keys become characters
- buffering that is no longer truly hardware-specific

That makes the keyboard layer too high-level.

The input system solves that by owning the translation/buffering step while
still staying below TTY semantics like raw/cooked mode and line editing.

## Unix-Like Direction

This design aims to be compatible with a Unix-like layering model:

- keyboard driver handles raw device input
- input system turns device input into terminal input bytes
- TTY interprets those bytes according to terminal mode
- `read()` returns the resulting byte stream

The input system in this project is not meant to be a full Linux input-event
subsystem. It is a smaller, byte-oriented layer appropriate for the current
single-console OS.

## Output Unit: Why The Buffer Stores Bytes

The input ring buffer should store `uint8_t` terminal input bytes.

Reasons:

- TTY and `read()` naturally consume bytes
- future special keys can expand into multiple bytes
- readers can consume one byte at a time if desired
- it keeps the first implementation small and practical

Examples:

- `a` press -> enqueue `'a'`
- Shift press -> enqueue nothing
- Shift release -> enqueue nothing
- Enter -> enqueue `'\n'`
- Backspace -> enqueue `'\b'`
- future arrow-up -> enqueue `'\x1b'`, `'['`, `'A'`

So the input system may emit:

- zero bytes
- one byte
- multiple bytes

for a single incoming scancode.

## Module Boundaries

### Keyboard Driver Responsibilities

The keyboard driver owns:

- reading port `0x60`
- IRQ1 registration
- forwarding raw scancodes into the input system

The keyboard driver should not own:

- the main input ring buffer
- translation into terminal bytes
- Shift state for terminal semantics
- line editing
- echo

### Input System Responsibilities

The input system owns:

- translation from raw scancode to terminal bytes
- modifier state such as Shift
- the byte ring buffer
- blocking and non-blocking byte reads

The input system should not own:

- VGA output
- TTY cooked/raw mode
- file descriptor routing
- shell or command semantics

### TTY Responsibilities

The TTY layer owns:

- raw mode vs cooked mode
- line buffering
- echo
- terminal editing behavior

The TTY should consume bytes produced by the input system.

## Proposed Files

- `include/input/input.h`
- `src/input/input.c`
- `include/input/input_ring.h`
- `src/input/input_ring.c`

The input ring should reuse the existing keyboard ring implementation that is
already present in the repo and covered by hosted unit tests.

Recommended move:

- `include/drivers/keyboard_ring.h` -> `include/input/input_ring.h`
- `src/drivers/keyboard_ring.c` -> `src/input/input_ring.c`
- `tests/unit/test_keyboard_ring.c` -> `tests/unit/test_input_ring.c`

Recommended renames:

- `struct keyboard_ring` -> `struct input_ring`
- `KEYBOARD_RING_CAPACITY` -> `INPUT_RING_CAPACITY`
- `keyboard_ring_init` -> `input_ring_init`
- `keyboard_ring_is_empty` -> `input_ring_is_empty`
- `keyboard_ring_is_full` -> `input_ring_is_full`
- `keyboard_ring_push` -> `input_ring_push`
- `keyboard_ring_pop` -> `input_ring_pop`

Why reuse is preferred:

- the ring semantics already match the byte-buffer design
- it is already implemented
- it already has hosted tests
- it is not currently wired into the live keyboard path

So this task should move and rename the existing ring rather than creating a
new buffer implementation from scratch.

## Proposed Public API

The first input-system API should be:

```c
void input_init(void);
void input_handle_scancode(uint8_t scancode);
bool input_has_byte(void);
bool input_read_byte(uint8_t *value);
uint8_t input_read_byte_blocking(void);
```

Behavior:

- `input_init()` resets modifier state and clears the buffer
- `input_handle_scancode()` processes one scancode and may enqueue zero or more bytes
- `input_has_byte()` reports whether translated input is buffered
- `input_read_byte()` pops one byte if available
- `input_read_byte_blocking()` spins until one byte is available

This API is intentionally byte-oriented and TTY-friendly.

## Internal State

The first implementation likely needs:

```c
struct input_state {
    struct input_ring ring;
    bool shift_pressed;
};
```

The state can be:

- module-private singleton state for now

That is acceptable because:

- there is only one keyboard/input path today
- this layer is not yet per-process or per-device

If the project later grows multiple input devices, this can be generalized.

## Translation Rules For The First Slice

The first input implementation should only support the subset already used by
the project and tests.

Minimum supported behavior:

- printable letters
- digits and basic punctuation already supported by the current table
- Shift press / release
- Enter -> `'\n'`
- Backspace -> `'\b'`
- ignore unsupported scancodes
- ignore release scancodes except where they update modifier state

This is enough to support the next TTY milestone without overbuilding.

## Suggested Translation Flow

For each scancode passed to `input_handle_scancode(scancode)`:

1. if it is Shift press, set `shift_pressed = true`, enqueue nothing
2. if it is Shift release, set `shift_pressed = false`, enqueue nothing
3. if it is another release code, enqueue nothing
4. if it is an unsupported make code, enqueue nothing
5. otherwise translate it to one byte
6. if Shift applies, adjust the translated byte
7. enqueue the resulting byte

Future enhancement:

- some keys may expand into multiple bytes rather than one

The enqueue side should already be designed so this is possible.

## Ring Buffer Policy

Use a fixed-size ring buffer of bytes.

Recommended first policy:

- on overflow, drop newest input byte

That keeps behavior simple and deterministic.

The overflow policy should be documented clearly in code comments and tests.

## Blocking Behavior

`input_read_byte_blocking()` should be a simple busy-wait for now:

```c
while (!input_read_byte(&value)) {
}
return value;
```

This is acceptable for the current kernel stage.

Later it can be replaced by a scheduler-aware wait mechanism.

## Testing Strategy

This module should be tested in isolation before TTY integration.

Recommended test split:

### Ring Tests

- initialization starts empty
- push/pop preserves FIFO order
- full buffer rejects or drops according to policy
- wraparound works correctly

These should be preserved by moving the current ring tests rather than
rewriting them from scratch.

### Translation/Input Tests

For `input.c`:

- `input_init()` resets state
- `a` press enqueues `'a'`
- Shift + `a` enqueues `'A'`
- Shift release updates state
- release of normal key enqueues nothing
- Enter enqueues `'\n'`
- Backspace enqueues `'\b'`
- unsupported scancode enqueues nothing
- blocking read returns the expected byte

These should be hosted unit tests, not e2e tests.

## Integration Plan

After this module exists and is tested:

1. update the keyboard IRQ path to call `input_handle_scancode(scancode)`
2. stop printing from IRQ context
3. implement the TTY module above `input_read_byte_blocking()`
4. later add `read(0, ...)`

So the input system is the immediate prerequisite for the TTY work.

## Definition Of Done

This task is complete when:

- `input.c` / `input.h` exist
- the input system accepts raw scancodes
- scancodes are translated into terminal input bytes
- translated bytes are stored in a ring buffer
- blocking and non-blocking byte reads work
- Shift, Enter, and Backspace behave as documented
- the module has hosted unit coverage

## Explicit Non-Goals

This task should not include:

- terminal cooked/raw mode
- echo or screen output
- `read(fd, ...)`
- per-process input routing
- generalized input-event objects
- mouse or other device support
- full Linux-style input subsystem complexity

Those belong to later milestones.
