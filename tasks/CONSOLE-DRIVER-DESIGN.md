# Console Driver Design

## Scope

This document defines the console driver as the terminal output backend for the
current OS.

It is responsible for rendering terminal output to a display backend and for
acting as the first concrete output target behind `tty_write()`.

This task is only about the console/output backend itself. It does not include:

- file descriptor tables
- process management
- `kwrite(fd, ...)` routing
- multiple consoles
- serial output
- user mode

## Goal

Introduce a dedicated console driver that:

1. accepts a stream of output bytes
2. interprets basic terminal control bytes such as newline and backspace
3. updates cursor position and scrolling state
4. renders through a display abstraction

The intended output path becomes:

`process or kernel code -> stdout -> tty_write() -> console_write() -> display`

And cooked input echo becomes:

`input -> tty_read(cooked) -> tty_write(echo) -> console_write() -> display`

## Why This Layer Exists

The TTY should own terminal semantics, but it should not know about VGA memory,
screen cell layout, or cursor registers.

The console driver should own:

- cursor movement on screen
- scrolling
- rendering of printable bytes and basic control bytes

The display backend should own:

- writing cells to the real device buffer
- synchronizing the hardware cursor if needed
- device-specific details such as VGA ports and memory layout

That keeps the TTY portable across output backends and keeps the console driver
focused on terminal-visible output behavior.

## High-Level Layering

The intended layering is:

- input system
- TTY
- console driver
- display backend
- VGA text buffer

For output specifically:

1. callers write bytes to `stdout`
2. `stdout` is wired to a TTY
3. `tty_write()` forwards bytes to the output backend
4. the console driver receives those bytes
5. the console driver renders them through the display backend

## Responsibilities

### TTY Responsibilities

TTY owns:

- terminal input semantics
- cooked/raw behavior
- echo policy
- calling the output backend via `tty_write()`

TTY should not own:

- direct VGA memory access
- hardware cursor register details
- scrolling implementation

### Console Driver Responsibilities

The console driver owns:

- current screen cursor position
- newline handling on screen
- backspace rendering on screen
- scrolling when the cursor moves past the last row
- issuing display operations for cells and cursor movement

The console driver should not own:

- keyboard input semantics
- line editing policy
- terminal mode
- file descriptor routing
- raw VGA port details

### Display Responsibilities

The display backend owns:

- writing a character cell to the underlying screen buffer
- moving the hardware cursor
- device-specific setup and buffer representation

The display backend should not own:

- terminal byte semantics
- newline policy
- scrolling policy

## Proposed Files

- `include/console/display.h`
- `src/console/display.c`
- `include/console/console.h`
- `src/console/console.c`

Important: this should be a brand new layer for now.

The legacy `include/console/console.h` and `src/console/console.c` should stay
untouched during this task. They represent the old VGA-coupled path and are
not the base for the new design.

The new terminal-facing console backend should therefore be introduced under a
new name such as `text_console`, with VGA specifics deferred to a future
`vga_display` backend.

## Proposed Display Object

Suggested first shape:

```c
struct display {
    uint16_t *buffer;
    uint16_t width;
    uint16_t height;
    void (*write_cell)(struct display *display, uint16_t row, uint16_t column, uint8_t ch, uint8_t color);
    void (*set_cursor)(struct display *display, uint16_t row, uint16_t column);
};
```

Why this is better:

- hosted tests can pass a mock buffer and mock cursor sink
- the real VGA backend can update both the text buffer and cursor ports
- console logic stays testable without depending directly on port I/O

For the first slice, the default VGA-backed initialization path can still set:

- buffer = `(uint16_t *)0xB8000`
- width = `80`
- height = `25`

## Proposed Console Object

Suggested first shape:

```c
struct text_console {
    struct display *display;
    uint16_t row;
    uint16_t column;
    uint8_t color;
};
```

The text console owns cursor semantics, but delegates the actual device update
to the display backend.

## Proposed Public API

The minimal text-console API should be:

```c
void text_console_init(struct text_console *console, struct display *display);
int text_console_write(struct text_console *console, const char *buf, int len);
void text_console_clear(struct text_console *console);
```

The first writer shape is intentionally simple:

- byte stream in
- number of bytes accepted out

This matches the backend shape needed by `tty_write()`.

The TTY output writer adapter can be:

```c
int text_console_tty_write(void *ctx, const char *buf, int len);
```

Where `ctx` is a `struct text_console *`.

## Byte Semantics

The text console should interpret at least:

- printable ASCII bytes
- `'\n'`
- `'\b'`

Recommended first behavior:

### Printable bytes

- render the character at the current cursor
- advance cursor by one column
- wrap to next line if needed

### Newline `'\n'`

- move cursor to the beginning of the next line
- scroll if already on the last line

### Backspace `'\b'`

- if not at the top-left origin, move cursor back one character position
- overwrite that cell with space
- leave cursor at the erased position

That backspace behavior matches what the TTY echo sequence `"\b \b"` expects:

1. first `'\b'` moves left
2. `' '` overwrites the old character
3. second `'\b'` moves left again

So the console driver must treat backspace as cursor movement plus erase-ready
positioning, not as a printable character.

## Cursor Handling

The text console should own a logical cursor in `struct text_console`.

When that logical cursor changes, the console driver should call:

- `display->set_cursor(display, row, column)`

The real VGA backend can then translate that into port writes.

## Scrolling

The driver should scroll when output reaches beyond the last visible row.

Simple first behavior:

- move rows `1..24` up to `0..23`
- clear the last row
- keep cursor on the last row

That is enough for the current stage.

## VGA Coupling

The VGA specifics should move into the display backend, not remain in the
terminal-facing console logic.

This gives you:

- a testable console layer
- a concrete VGA display backend for the real OS
- a clean place for cursor-port writes

## Integration With TTY

The TTY should not call ad hoc console helper functions directly.

Instead, it should be wired through the output-writer interface:

```c
tty_init(&kernel_tty, input_read_byte_blocking, text_console_tty_write, &kernel_text_console);
```

Where:

```c
int text_console_tty_write(void *ctx, const char *buf, int len) {
    return text_console_write((struct text_console *)ctx, buf, len);
}
```

That keeps the TTY generic and makes the console driver a backend.

## Testing Strategy

The text console should be tested separately from TTY.

Recommended first approach:

- hosted/unit tests for byte-stream behavior against a mock display/buffer
- later e2e confirmation through VGA memory in QEMU

Suggested unit coverage:

- writing one printable character advances cursor
- writing a string lands bytes in successive cells
- newline moves to next row start
- backspace erases the previous cell
- wrapping at end of line works
- scrolling at bottom row works

The display abstraction is the test seam:

- tests provide a mock display
- the real OS provides a VGA-backed display

## Suggested Implementation Order

1. introduce `display.h` / `display.c`
2. introduce `text_console.h` / `text_console.c`
3. define `text_console_init(struct text_console *, struct display *)`
4. define `text_console_write(struct text_console *, const char *buf, int len)`
5. ensure newline and backspace semantics match TTY expectations
6. provide `text_console_tty_write(void *ctx, const char *buf, int len)`
7. later wire the kernel TTY instance to the new console backend

## Definition Of Done

This console-driver task is done when:

- the new text-console module exposes a byte-stream write API
- the new text-console module depends on a display abstraction rather than hardcoded VGA globals
- the new text-console module can act as the output backend for TTY
- printable bytes, newline, and backspace render correctly
- cursor updates and scrolling are handled by the text console
- no TTY code writes directly to VGA memory

## Explicit Non-Goals

This task should not include:

- ANSI escape-sequence parsing
- multiple virtual consoles
- color control sequences
- terminal resizing
- serial console backends
- file descriptor routing
- process-level stdout wiring

Those belong to later milestones.
