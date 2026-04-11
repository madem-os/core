# Usermode Stage 1: Kernel I/O And Process Wiring

## Scope

This document defines the first implementation slice toward user mode.

This stage does not enter ring 3 yet. It introduces the kernel-side objects and
APIs that user mode will later rely on:

- a minimal file descriptor model
- a minimal process object
- kernel-side `read` / `write` dispatch
- wiring of stdin/stdout/stderr to the current TTY

The goal is to establish the ABI boundary in kernel space first, before adding
syscall entry and user-mode transition.

## Goal

After this stage:

- the kernel has one `current_process`
- that process has `fd 0`, `fd 1`, `fd 2`
- all three descriptors are wired to the active TTY
- `kread(fd, ...)` dispatches input correctly
- `kwrite(fd, ...)` dispatches output correctly
- the kernel main flow can use `kread` / `kwrite` instead of touching TTY
  directly

This creates the minimal OS-facing interface that later syscalls will expose to
user mode.

## Non-Goals

This stage does not include:

- ring 3 transition
- syscall interrupt entry
- user-space binaries
- paging or address-space isolation
- multiple processes
- pipes
- files
- redirection
- process scheduling

## Proposed Files

New files:

- `include/kernel/process.h`
- `src/kernel/process.c`
- `include/kernel/io.h`
- `src/kernel/io.c`
- `tests/unit/test_kernel_io.c`

Existing files likely to change:

- `kernel.c`
- possibly `tests/run-unit.sh`

## Minimal Types

### `include/kernel/process.h`

```c
#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stddef.h>

enum fd_kind {
    FD_KIND_NONE = 0,
    FD_KIND_TTY = 1
};

struct file_descriptor {
    enum fd_kind kind;
    void *object;
};

struct process {
    struct file_descriptor fds[3];
};

void process_init(struct process *process);
void process_set_tty_stdio(struct process *process, void *tty);
struct process *process_current(void);
void process_set_current(struct process *process);

#endif
```

Notes:

- `void *object` is enough for stage 1.
- only `fds[0]`, `fds[1]`, `fds[2]` exist for now
- `FD_KIND_TTY` is the only supported descriptor kind in this stage

### `include/kernel/io.h`

```c
#ifndef KERNEL_IO_H
#define KERNEL_IO_H

int kread(int fd, char *buf, int len);
int kwrite(int fd, const char *buf, int len);

#endif
```

## Implementation Responsibilities

### `src/kernel/process.c`

Responsibilities:

- initialize a process object
- assign stdin/stdout/stderr to a TTY object
- track the one global `current_process`

Expected functions:

```c
void process_init(struct process *process);
void process_set_tty_stdio(struct process *process, void *tty);
struct process *process_current(void);
void process_set_current(struct process *process);
```

Expected behavior:

- `process_init()` clears all fd slots to `FD_KIND_NONE`
- `process_set_tty_stdio()` sets:
  - `fds[0] = { FD_KIND_TTY, tty }`
  - `fds[1] = { FD_KIND_TTY, tty }`
  - `fds[2] = { FD_KIND_TTY, tty }`
- `process_set_current()` stores the global current-process pointer
- `process_current()` returns it

### `src/kernel/io.c`

Responsibilities:

- route `kread` and `kwrite` through the current process fd table
- dispatch TTY-backed descriptors to `tty_read` / `tty_write`
- reject unsupported fds cleanly

Expected functions:

```c
int kread(int fd, char *buf, int len);
int kwrite(int fd, const char *buf, int len);
```

Expected behavior:

- `kread()`:
  - validate `fd`, buffer, and length
  - look up `process_current()->fds[fd]`
  - if `FD_KIND_TTY`, call `tty_read((struct tty *)object, buf, len)`
  - otherwise return `-1`

- `kwrite()`:
  - validate `fd`, buffer, and length
  - look up `process_current()->fds[fd]`
  - if `FD_KIND_TTY`, call `tty_write((struct tty *)object, buf, len)`
  - otherwise return `-1`

Recommended stage-1 fd policy:

- allow reads only from `fd 0`
- allow writes only to `fd 1` and `fd 2`
- return `-1` for unsupported direction or invalid fd

That keeps the behavior clear even though all three currently point to the same
TTY object.

## Kernel Wiring

`kernel.c` should be updated to:

1. create one `struct process`
2. initialize it
3. wire stdio to the active TTY
4. set it as the current process
5. use `kwrite(1, ...)` for boot output
6. use `kread(0, ...)` for input

Suggested kernel-owned objects:

```c
static struct tty kernel_tty;
static struct text_console kernel_text_console;
static struct display kernel_display;
static struct process kernel_process;
```

Suggested initialization flow:

```c
input_init();
keyboard_init(input_handle_scancode);

init_vga_display(&kernel_display, 0);
text_console_init(&kernel_text_console, &kernel_display);
tty_init(
    &kernel_tty,
    input_read_byte_blocking,
    text_console_tty_write,
    &kernel_text_console
);

process_init(&kernel_process);
process_set_tty_stdio(&kernel_process, &kernel_tty);
process_set_current(&kernel_process);
```

Suggested usage:

```c
kwrite(1, "welcome to Madem-OS!\n", 21);
kread(0, buf, sizeof(buf));
```

## Testing Strategy

### Hosted Unit Tests

Add a new hosted test file:

- `tests/unit/test_kernel_io.c`

The test should verify:

1. `process_init()` clears fd slots
2. `process_set_tty_stdio()` wires all three stdio slots to the provided TTY
3. `kread(0, ...)` dispatches to the TTY read path
4. `kwrite(1, ...)` dispatches to the TTY write path
5. `kwrite(2, ...)` dispatches to the same TTY write path
6. invalid fd returns `-1`
7. read from stdout/stderr returns `-1`
8. write to stdin returns `-1`

The easiest test seam is:

- construct a real `struct tty`
- inject mock input-reader / output-writer callbacks into it
- assign that TTY to the process stdio slots
- assert that `kread` / `kwrite` reach the expected TTY behavior

No QEMU changes are required for this stage.

## Design Choices

### Why `src/kernel/`

This logic is:

- not x86-specific
- not TTY-internal
- not console-internal

So it belongs in a generic kernel layer.

Later, x86 syscall entry code should call into this layer rather than calling
TTY directly.

### Why Use `void *object` In `struct file_descriptor`

At this stage, the only object kind is TTY.

Using `void *` keeps the structure minimal while still modeling the correct
indirection:

- fd table entry
- object kind
- underlying endpoint object

This can later grow into a fuller file/open-object model without rewriting the
callers.

### Why Keep Terminal Mode On TTY

This stage must not move raw/cooked mode onto the process.

TTY mode remains a property of the TTY object. The process only holds a
descriptor pointing to that object.

That is the right long-term direction for:

- redirected stdin
- pipes
- multiple TTYs

## Definition Of Done

This stage is done when:

- `src/kernel/process.c` exists and provides current-process + stdio wiring
- `src/kernel/io.c` exists and provides `kread` / `kwrite`
- `kernel.c` uses `kread` / `kwrite` instead of calling TTY directly
- hosted unit tests cover the dispatch behavior
- existing unit tests still pass
- the kernel still builds and boots

