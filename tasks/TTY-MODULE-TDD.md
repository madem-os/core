# TTY Module TDD Task

## Scope

This task is only about implementing the first standalone `tty` module.

It does not include:

- wiring the TTY into `read(0, ...)`
- integrating the TTY with the live keyboard driver
- changing the IRQ1 path
- changing the main kernel loop
- changing the e2e test flow

The module should be developed and verified in isolation first.

## Goal

Create a minimal `tty` module that operates on `struct tty` and supports:

- raw mode
- cooked mode
- mode switching
- cooked-mode echo
- blocking reads based on an injected input-byte source
- terminal output through an injected byte-stream writer

This work must be done with TDD:

1. add the public API and stub implementation
2. add hosted unit tests for the target behavior
3. verify that the tests fail against the stubs
4. implement the module until the tests pass

The point of this task is to lock down TTY behavior before integrating it into
the kernel input path.

## Why This Slice Exists

The larger roadmap now depends on a TTY layer that sits between:

- translated input bytes from the input system
- future `read(0, ...)` semantics

If we integrate the TTY directly into the kernel flow before testing it in
isolation, then failures will be harder to localize. We want the TTY rules to
be stable before they are mixed with:

- keyboard IRQ behavior
- input-system translation and buffering
- console echo from the main kernel loop

## Deliverables

Add:

- `include/tty/tty.h`
- `src/tty/tty.c`
- hosted unit tests for the TTY module under `tests/unit/`

The implementation should build cleanly in the existing hosted unit-test
infrastructure.

## Required TDD Workflow

### Step 1: Add API Header

Create the public API first in `include/tty/tty.h`.

The first version should define:

```c
typedef uint8_t (*tty_input_reader_t)(void);
typedef int (*tty_output_writer_t)(void *ctx, const char *buf, int len);

enum tty_mode {
    TTY_MODE_COOKED = 0,
    TTY_MODE_RAW = 1
};

struct tty {
    enum tty_mode mode;
    bool echo_enabled;
    tty_input_reader_t read_input_byte_blocking;
    tty_output_writer_t write_output;
    void *output_ctx;
};

void tty_init(
    struct tty *tty,
    tty_input_reader_t reader,
    tty_output_writer_t writer,
    void *output_ctx
);
void tty_set_mode(struct tty *tty, enum tty_mode mode);
enum tty_mode tty_get_mode(const struct tty *tty);
char tty_read_raw_blocking(struct tty *tty);
int tty_read(struct tty *tty, char *buf, int len);
int tty_write(struct tty *tty, const char *buf, int len);
```

The API can grow later, but this is the minimum target for the first TTY
module slice.

### Step 2: Add Stub Implementation

Create `src/tty/tty.c` with a complete stub implementation that compiles but
does not satisfy the tests.

For example:

- `tty_init()` stores default state
- `tty_set_mode()` may be empty at first
- `tty_get_mode()` may return a placeholder
- `tty_read_raw_blocking()` may return `0`
- `tty_read()` may return `0`
- `tty_write()` may return `0`
- output writer may be stored but not used yet

The important part is that the code compiles and the tests fail because the
behavior is not implemented yet.

### Step 3: Add Hosted Unit Tests

Add unit tests before implementing real logic.

The tests should inject a fake input-byte reader rather than using the real
input system.

Each test should construct its own `struct tty` instance and initialize it
with a test reader and mock output writer.

Do not use hidden singleton state in the TTY module.

### Step 4: Confirm Initial Failure

Before implementing the real logic, run the hosted test script and confirm the
new TTY tests fail for the expected reasons.

That failure is required for this task. It proves the tests are meaningful and
the implementation is not accidentally already satisfying them.

### Step 5: Implement Until Green

After the failing test phase is confirmed, implement the TTY module until all
TTY unit tests pass.

This task is complete when the hosted tests pass. Kernel integration is a
separate task.

## Test Strategy

The hosted TTY tests should use a deterministic fake input-byte source.

Recommended pattern:

- test file owns a static array of input bytes
- fake reader returns the next byte from that array
- each test resets the array index before use
- fake writer appends to a test output buffer

That allows precise testing of:

- immediate byte reads
- Enter
- Backspace
- mode switching
- output forwarding through `tty_write()`
- cooked-mode echo behavior

## Initial Behavior To Test

The tests should cover at least the following cases.

### 1. `tty_init()` defaults to cooked mode

After initialization:

- mode should be `TTY_MODE_COOKED`
- echo should default to enabled
- the injected reader should be stored
- the injected output writer should be stored

### 2. `tty_set_mode()` and `tty_get_mode()` work

Switch from cooked to raw and back, and verify the visible mode value changes
correctly.

### 3. Raw mode returns the next byte immediately

Given input byte sequence such as `a`, raw mode should return `'a'` without
waiting for Enter.

### 4. Cooked mode blocks until Enter completes the line

Given:

- `a`
- `b`
- Enter

`tty_read()` in cooked mode should return the completed line only after Enter.

The exact return convention should be chosen and documented:

- return count including newline, or
- return count excluding newline

Either is acceptable for this task as long as the tests fix the contract.

### 5. Cooked mode handles Backspace

Given:

- `a`
- `b`
- Backspace
- `c`
- Enter

the resulting line should be `ac` or `ac\n`, depending on the chosen newline
policy.

If echo is enabled, the output side should emit the visual erase sequence
`"\b \b"` when the backspace is processed.

### 6. `tty_read()` dispatches according to mode

Verify that:

- in raw mode, `tty_read()` returns immediate character input
- in cooked mode, `tty_read()` returns edited line input

This test is important because later `read(0, ...)` will depend on it.

### 7. `tty_write()` forwards bytes to the output backend

The tests should verify that terminal output goes through the injected byte
writer rather than through VGA-specific callbacks.

At minimum, verify:

- `tty_write()` passes the provided bytes through unchanged
- the writer receives the same byte sequence
- the return value is propagated correctly

### 8. Cooked mode echoes through `tty_write()`

The tests should verify that cooked reads produce visible output when echo is
enabled.

At minimum, verify:

- normal printable input echoes the same bytes
- Enter echoes `'\n'`
- Backspace echoes `"\b \b"`

## Suggested Minimal Internal Design

The TTY module may keep only a small amount of state in `struct tty` for this
first slice.

Reasonable first fields:

```c
struct tty {
    enum tty_mode mode;
    bool echo_enabled;
    tty_input_reader_t read_input_byte_blocking;
    tty_output_writer_t write_output;
    void *output_ctx;
};
```

If cooked line buffering needs extra temporary storage, it can live inside
`tty_read()` as a local buffer strategy for now, or be stored in the TTY if
that makes the implementation cleaner.

The exact choice is left to the implementer as long as the public contract and
tests stay clear.

## Input Assumptions For This Task

The TTY module should assume that translation from scancode to terminal input
bytes already happened in the input system.

Expected input to the TTY:

- printable bytes
- `'\n'` for Enter
- `'\b'` for Backspace

This task is not the place to add keyboard-layout translation.

## Output Behavior

The TTY module should not depend directly on `console.c` or raw VGA memory.

Instead, it should expose `tty_write()` and use an injected byte-stream writer
backend. That gives the module a more terminal-like shape and keeps the output
backend replaceable.

For this task:

- production code will later provide a console-backed writer
- hosted unit tests should provide a mock writer
- tests should assert against recorded output bytes
- direct VGA buffer assumptions are not part of the TTY API

Cooked-mode echo should use this writer path rather than special display-only
callbacks. That keeps the design closer to a real terminal byte stream.

## Definition Of Done

This TTY module task is done when:

- `include/tty/tty.h` exists with the agreed API
- `src/tty/tty.c` exists
- the implementation was introduced via stub first
- hosted unit tests were added before the full implementation
- the initial stubbed version fails those tests
- the final implementation makes all TTY unit tests pass
- no kernel integration changes were required for this task

## Explicit Non-Goals

Do not include any of the following in this task:

- live keyboard driver integration
- `read(fd, ...)` kernel API
- TTY ownership by a process object
- TTY `ioctl`/`tcsetattr` support
- multiple TTY instances in the running kernel
- shell integration
- e2e test updates

Those belong to later tasks.
