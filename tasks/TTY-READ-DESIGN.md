## TTY Read Path Design

### Scope

This document describes the TTY read path after the introduction of a separate
input system. It is no longer the immediate next implementation slice.

The goal is to place a TTY layer above translated input bytes rather than
above raw scancodes.

This task intentionally does not introduce:

- a full process model
- multiple TTYs
- file redirection
- pipes
- user mode
- syscalls

Those can come later without changing the core layering introduced here.

### Problem Statement

The current keyboard path still mixes interrupt handling and console output.
Today the IRQ1 handler eventually causes characters to be written directly to
the VGA console. That proved the interrupt path works, but it is the wrong
long-term structure.

We want the architecture to be closer to a Unix-like model:

- keyboard IRQ code handles hardware input only
- the keyboard driver forwards raw scancodes
- an input system translates scancodes into terminal input bytes and buffers them
- a TTY layer interprets that byte stream according to mode
- `read(fd, ...)` consumes from the TTY, not from interrupt side effects

This must still preserve a passing end-to-end test after the next commit.

### Design Principles

- Keep IRQ1 minimal and deterministic.
- Buffer terminal input bytes in the input system, not in the keyboard driver.
- Keep terminal mode on a `struct tty`, not on a process.
- Keep keyboard hardware handling separate from console and line editing.
- Preserve a Unix-like direction even before processes and syscalls exist.
- Keep the first TTY implementation intentionally small.

### Why The Ring Buffer Should Move To The Input System

If the keyboard driver owns the main input ring buffer, then the keyboard layer
also ends up owning translation policy and terminal-facing semantics.

That mixes responsibilities that should stay separate:

- keyboard driver: raw hardware input
- input system: translation to terminal bytes and buffering
- TTY: raw/cooked terminal semantics and echo policy

The input system should therefore own a byte-oriented ring buffer. It can
translate one scancode into zero, one, or multiple output bytes before the TTY
consumes them.

### High-Level Layering

The target stack for this slice is:

1. keyboard IRQ path
2. keyboard driver raw scancode forwarding
3. input system translation and byte ring buffer
4. TTY object with raw/cooked mode
5. `read(fd, ...)` dispatch for `stdin`
6. main kernel loop consuming `read(0, ...)` and writing to console

The intended data flow is:

`IRQ1 -> keyboard driver -> input system byte buffer -> TTY raw/cooked
interpretation -> read(0, ...) -> console output`

### Module Ownership

#### Keyboard Driver

The keyboard driver owns:

- IRQ1 registration
- reading the data port
- forwarding raw scancodes into the input system

It should not own:

- the main input ring buffer
- cooked line editing
- console echo policy
- `stdin` semantics
- terminal mode state

#### Input System

The input system owns:

- translation of scancodes into terminal input bytes
- modifier state such as Shift
- the byte ring buffer
- blocking and non-blocking reads of translated bytes

It should not own:

- console rendering
- cooked terminal policy
- file descriptor routing

#### TTY

The TTY layer owns:

- current mode: raw or cooked
- whether terminal echo is enabled
- interpretation of terminal input bytes according to terminal mode
- backspace and Enter behavior in cooked mode
- deciding how `read(0, ...)` behaves
- terminal output writes via a byte-stream writer backend

It should not own:

- hardware port reads
- IRQ setup
- PIC logic
- VGA internals

#### Console

The console module remains output-focused:

- write one character
- write strings
- cursor movement
- newline/backspace rendering

It should not own keyboard IRQ behavior or input buffering.

### TTY State Placement

Terminal mode should not be stored on a future process object.

In Unix-like systems, raw/cooked behavior belongs to the terminal endpoint,
not to the abstract process. A process may read from a TTY, file, or pipe
through file descriptor `0`. Only the TTY-backed case supports terminal mode.

For this project stage, the correct approximation is:

- a `struct tty`
- one kernel-owned instance used by `stdin` for now
- mode stored on that object

Later, when processes and file descriptors exist, fd `0` can point to the
active TTY object. If `stdin` later points to a file or pipe, terminal mode
operations should fail for that fd.

### Proposed Modules

- `src/drivers/keyboard.c`
- `include/drivers/keyboard.h`
- `src/input/input.c`
- `include/input/input.h`
- `src/tty/tty.c`
- `include/tty/tty.h`
- optional later wrapper:
  - `src/kernel_io/read.c`
  - `include/kernel_io/read.h`

For this slice, `read(fd, ...)` can also live in a small kernel I/O module or
in `tty.c` if you want to keep the tree small. A dedicated module is cleaner.

### Keyboard API

The keyboard layer should expose initialization and a way to hand raw scancodes
into the input system, but it should not expose the main input buffer to TTY.

```c
void keyboard_init(void);
void keyboard_handle_scancode(uint8_t scancode);
```

Behavior:

- IRQ1 reads the raw byte from port `0x60`
- the keyboard driver forwards it into the input system
- no terminal-facing reads happen at this layer

### Input API

The input layer should expose translated byte reads:

```c
void input_init(void);
void input_handle_scancode(uint8_t scancode);
bool input_has_byte(void);
bool input_read_byte(uint8_t *value);
uint8_t input_read_byte_blocking(void);
```

Behavior:

- `input_handle_scancode()` updates translation state and may enqueue zero,
  one, or multiple terminal input bytes
- `input_read_byte_blocking()` spins until one translated byte is available

### TTY API For This Slice

The first TTY API should be small, explicit, and operate on `struct tty`.

The TTY depends on a blocking input-byte source injected at initialization time.
That keeps the module easy to unit-test without requiring the real input-system
integration in the same commit.

```c
typedef uint8_t (*tty_input_reader_t)(void);

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

typedef int (*tty_output_writer_t)(void *ctx, const char *buf, int len);

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

Notes:

- `tty_read_raw_blocking()` returns the next immediate input character
- it is built on top of `input_read_byte_blocking()`
- `tty_read()` is the entry used by `read(0, ...)`
- in raw mode, `tty_read()` should return as soon as characters are available
- in cooked mode, `tty_read()` should return one edited line after Enter
- cooked-mode echo should go through `tty_write()`, not through keyboard/input layers

If wanted, a helper such as `tty_read_cooked_blocking()` can exist privately
inside `tty.c`.

### Raw Mode Semantics

Raw mode in this project should mimic Unix terminal raw mode conceptually.

That means:

- the source of truth for TTY is the translated input byte stream
- `read()` returns input bytes/characters, not release events
- no cooked line buffering
- no cooked backspace handling
- no forced newline-delimited reads

This is analogous to Unix raw terminal input, where `read()` yields bytes.

### Cooked Mode Semantics

Cooked mode should be minimal but deterministic.

For this slice:

- printable characters append to a line buffer
- if `echo_enabled` is true, printable characters are echoed through `tty_write()`
- Backspace removes one buffered character if possible
- if `echo_enabled` is true, Backspace should emit the visual erase sequence `"\b \b"`
- Enter finalizes the line
- if `echo_enabled` is true, Enter should emit `"\n"` through `tty_write()`
- `tty_read()` returns after Enter
- the returned buffer should be NUL-terminated if space allows

Whether newline is included in the returned data should be decided once and
documented. A Unix-like choice is to include `'\n'`. Either approach is fine
for now as long as it is consistent and tested.

### Translation Placement

Scancode-to-byte translation should happen in the input system, not in the TTY
and not in the IRQ path.

The TTY should consume a byte stream that is already suitable for terminal
semantics. Future special keys can still become multi-byte sequences at the
input layer.

### `read(fd, ...)` For This Slice

Introduce a regular kernel function:

```c
int read(int fd, char *buf, int len);
```

Temporary behavior:

- if `fd == 0`, dispatch to the kernel-owned TTY instance
- if `fd != 0`, return an error for now

TTY-backed behavior:

- if mode is raw, return available characters immediately
- if mode is cooked, block until a full line is ready

This is a kernel helper for now, not a syscall. Later it can become the
internal implementation behind a user-visible syscall.

### Main Loop Behavior

The main kernel loop must change so visible input still appears on screen even
though the IRQ path no longer writes to VGA directly.

Recommended temporary flow:

1. boot and print startup text
2. initialize keyboard, input, and TTY
3. repeatedly call `read(0, ...)`
4. or let the TTY drive echo through a console-backed writer

That preserves the current user-visible behavior while moving all echoing out
of interrupt context.

### End-To-End Test Requirement

The existing e2e flow should still pass after this task.

The test expectation can remain:

- boot text appears
- after `sendkey a`, `a` appears on screen

But the implementation path must become:

- IRQ1 reads scancode and forwards it
- input system translates and buffers terminal bytes
- main kernel read path consumes via TTY
- TTY echo reaches the console through the TTY output writer

The test should validate the new architecture indirectly by keeping the same
observable output while the interrupt path becomes simpler.

### Suggested Implementation Order

1. Introduce `input.c` / `input.h`.
2. Update `keyboard.c` so IRQ1 forwards raw scancodes only.
3. Add byte buffering and scancode translation to the input system.
4. Expose `input_read_byte_blocking()`.
5. Introduce `tty.c` / `tty.h` with `struct tty` state and injected blocking
   input-byte reader.
6. Add raw input path on top of input byte reads.
7. Add cooked input path with basic line editing.
8. Add `read(fd, ...)` and route `fd == 0` to the kernel-owned TTY instance.
9. Update the main kernel loop to consume `read(0, ...)` and echo output.

### Definition Of Done

This combined task is done when:

- IRQ1 no longer writes to the console
- IRQ1 only reads port `0x60` and forwards the raw scancode
- the input system translates and buffers terminal input bytes
- a `struct tty` implementation exists with raw/cooked mode
- the kernel owns one TTY instance used for `stdin`
- `read(0, ...)` dispatches through that TTY instance
- cooked mode returns edited line input
- visible keyboard echo happens from normal kernel code, not IRQ context
- the end-to-end test still passes

### Non-Goals

This task should not introduce:

- multiple terminals
- process scheduling
- per-process fd tables
- redirection semantics
- pipes
- terminal signals
- full POSIX `termios`

Those belong to later milestones once the first working TTY-backed read path
is stable.
