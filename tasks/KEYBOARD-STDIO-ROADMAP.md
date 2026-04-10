# Keyboard Driver And Stdio Roadmap

## Goal

Build a more standard input/output path for the OS before user mode by separating:

- keyboard hardware handling
- input translation and buffering
- console / TTY behavior
- `stdin` / `stdout` / `stderr` style interfaces

This should replace the current "IRQ prints directly to VGA" experiment with a structure that can later support a shell, built-in programs, and eventually user mode.

## Why This Comes Before User Mode

User mode without a basic I/O model would force early programs to know about VGA memory, PIC setup, and keyboard scancodes directly.

That does not scale.

The better order is:

1. keyboard driver
2. input system
3. console / TTY behavior
4. stream-like I/O interface
5. built-in applications using that interface
6. user mode and syscalls later

## Current State In This Repo

Today the code is still in a demo stage:

- keyboard IRQ1 is installed in `src/mos-cmd.c`
- the interrupt handler reads scancodes from port `0x60`
- scancodes are translated and printed directly by logic in `src/mos-stdio.c`
- input and output are tightly coupled

This proved that interrupts work, but it is not yet a reusable driver design.

## Milestone 1: Proper Keyboard Driver

### Task 1.1: Create a dedicated keyboard module

Add a driver module, for example:

- `src/drivers/keyboard.c`
- `include/drivers/keyboard.h`

Responsibilities:

- read keyboard scancodes from hardware
- own IRQ registration for the keyboard device
- forward raw scancodes to the next input-processing layer
- expose a small driver API

The keyboard driver should not own:

- terminal-style buffering
- line editing
- byte-stream semantics for `stdin`
- cooked/raw terminal mode

Do not leave keyboard logic inside `mos-stdio`.

### Task 1.2: Add an input system

Add an input module, for example:

- `src/input/input.c`
- `include/input/input.h`

Responsibilities:

- receive raw scancodes from the keyboard driver
- maintain translation state such as Shift
- translate scancodes into terminal input bytes
- own the input ring buffer
- expose blocking and non-blocking reads of translated bytes

Suggested first version:

- buffer type: `uint8_t`
- semantics: terminal input bytes
- size: 128 or 256 bytes
- indices: `head`, `tail`
- policy on full buffer: drop newest input for now

The input system may produce:

- zero bytes for scancodes like Shift press/release
- one byte for normal keys like `a`
- multiple bytes for future special keys such as arrows or function keys

### Task 1.3: Keep the interrupt handler minimal

Update the IRQ1 path so it does only:

1. read scancode from port `0x60`
2. pass it to the keyboard driver
3. send EOI to the PIC
4. return

The interrupt handler should not:

- print to VGA
- perform line editing
- translate into terminal bytes
- own console behavior

### Task 1.4: Expose a blocking input API

Add a simple kernel-facing API such as:

- `bool input_has_byte(void);`
- `bool input_read_byte(uint8_t *value);`
- `uint8_t input_read_byte_blocking(void);`

The input system should expose terminal-style byte input. This is a better
fit for a future TTY layer than buffering raw keyboard events in the terminal
path.

## Milestone 2: Console / TTY Layer

### Task 2.1: Separate console output from stdio naming

The current `mos-stdio` module is really a VGA console helper.

Refactor toward a console-oriented module that owns:

- write one character
- write a string
- newline handling
- cursor movement
- backspace handling

This can still use VGA text mode internally.

### Task 2.2: Add a console input layer above the input system

Build a layer that consumes terminal input bytes and provides terminal-like
behavior:

- character echo
- backspace editing
- Enter submits a line
- optional prompt rendering

This is closer to a TTY than raw keyboard handling.

### Task 2.3: Add line-buffered reads

Implement something like:

- `int console_read_line(char *buf, int max_len);`

Behavior:

- block until Enter
- echo typed characters
- handle backspace safely
- NUL-terminate the line

This becomes the first version of `stdin`.

## Milestone 3: Stdio-like Kernel Interface

### Task 3.1: Introduce logical streams 0, 1, 2

Define the model now, even if implementation is simple:

- `0` = `stdin`
- `1` = `stdout`
- `2` = `stderr`

Initial mapping:

- `stdin` -> console input layer
- `stdout` -> VGA console
- `stderr` -> VGA console for now

Later `stderr` can differ by color, device, or routing.

### Task 3.2: Add kernel read/write entry points

Add small kernel APIs with syscall-like shape, for example:

- `int kread(int fd, char *buf, int len);`
- `int kwrite(int fd, const char *buf, int len);`

At this stage they are just normal kernel functions.

The point is to make built-in applications stop depending on hardware details.

### Task 3.3: Convert command code to use the stream layer

Refactor the command loop so it reads from `stdin` and writes to `stdout`, rather than touching keyboard/VGA internals directly.

## Milestone 4: Built-In Programs

### Task 4.1: Add a minimal shell loop

Once line input exists, add a simple shell:

- show prompt
- read line
- parse command
- execute built-in command

Suggested first built-ins:

- `help`
- `clear`
- `echo`

### Task 4.2: Keep built-in programs device-agnostic

Built-ins should use the stream interface only.

They should not:

- read I/O ports
- manipulate VGA memory directly
- depend on interrupt internals

## Milestone 5: Prepare For User Mode Later

### Task 5.1: Distinguish raw input from cooked input

Plan for two future modes:

- raw mode: terminal input bytes are delivered directly without cooked editing
- cooked mode: line editing and echo are handled by the console / TTY layer

The shell should use cooked mode.

Future full-screen or interactive apps may want raw mode.

### Task 5.2: Preserve syscall-shaped APIs

If `kread` / `kwrite` are introduced now, they can later become the internal implementation behind user-mode syscalls.

That makes the eventual transition much cleaner.

### Task 5.3: Add a more general device model later

After keyboard and console are stable, introduce a device abstraction so more devices can plug into the same model:

- serial console
- timers
- disks
- pseudo terminals later

This should come after the first working input/output path, not before.

## Recommended First Implementation Slice

Build this exact slice first:

1. create `keyboard.c` and `keyboard.h`
2. create `input.c` and `input.h`
3. move scancode translation into the input system
4. add a byte ring buffer owned by the input system
5. update IRQ1 to forward scancodes instead of printing
6. add `input_read_byte_blocking()`
7. update the current command loop to read from the input system and echo using console output

This gives a real keyboard driver and a first input pipeline without
overdesigning the system.

## Definition Of Done For The First Milestone

The first milestone is complete when:

- IRQ1 no longer prints directly
- translated terminal input bytes are buffered
- the main loop consumes buffered input bytes
- normal typing works even if input briefly arrives faster than consumption
- Enter and Backspace work in a basic, predictable way
- keyboard, input, and console responsibilities are split into separate modules

## Notes

- Keep the interrupt path short and deterministic.
- Avoid dynamic allocation for this stage.
- Prefer simple single-producer / single-consumer assumptions for the ring buffer.
- Document buffer overflow behavior explicitly.
- Treat this as infrastructure work, not polish work.
