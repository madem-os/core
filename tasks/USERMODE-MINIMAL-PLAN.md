# Minimal User Mode Plan

## Scope

This document defines the first end-to-end plan for getting MademOS from the
current single-address-space kernel into a minimal single-process OS with:

- user mode
- a syscall entry path
- one user process
- stdin/stdout/stderr style file descriptors
- terminal applications that read from the keyboard path and write to the text
  console through the TTY stack

This plan is intentionally minimal. It is not a full multitasking or POSIX
design.

## Goal

The first user-mode milestone is done when the OS can:

1. boot into the kernel
2. initialize the current input, TTY, text-console, and VGA-display stack
3. create one user process
4. switch to ring 3 and run that process
5. let the process call into the kernel through syscalls
6. let the process use `read(0, ...)` and `write(1, ...)`
7. display program output on the text console and receive terminal input from
   the current keyboard/input/TTY path

The first concrete demo target should be a tiny terminal program such as:

- `echo`: read one cooked line and write it back
- or `hello`: write a string to stdout and then block on stdin

## Current Base

The current tree already has the right lower layers to build on:

- keyboard IRQ path
- input system
- TTY layer
- text console
- display abstraction
- VGA display backend
- hosted unit tests
- QEMU e2e boot/input/output tests

So this milestone is not starting from zero. The next work is about adding the
kernel ABI and execution boundary above those layers.

## Non-Goals

This first user-mode milestone does not include:

- multiple processes
- preemptive scheduling
- process isolation through paging
- ELF loading from a filesystem
- pipes
- redirection
- signals
- dynamic memory in userspace
- a shell parser or job control
- multiple TTYs

Those can come later.

## Design Principles

- Prefer a small, coherent vertical slice over a broad but half-finished OS
  API.
- Keep the current terminal stack intact and route the new process/stdin/stdout
  path through it rather than bypassing it.
- Separate responsibilities clearly:
  - syscall dispatch in one place
  - fd routing in one place
  - TTY policy in TTY
  - rendering in text console / VGA backend
- Start with one user process and a shared kernel/user address space if needed.
- Make the first ABI intentionally tiny and explicit.

## High-Level Architecture

The intended first-stack becomes:

`keyboard IRQ -> input -> tty -> text_console -> vga_display`

and:

`user process -> syscall -> fd layer -> tty -> text_console -> vga_display`

For input:

`keyboard IRQ -> input -> tty -> syscall read(fd=0) -> user buffer`

For output:

`user buffer -> syscall write(fd=1 or 2) -> tty_write -> text_console ->
vga_display`

## Recommended Stages

### Stage 1: Kernel I/O Boundary

Introduce a minimal kernel I/O layer that presents fd-oriented `read` and
`write` behavior inside the kernel first.

Target API shape:

```c
int kread(int fd, char *buf, int len);
int kwrite(int fd, const char *buf, int len);
```

Initial fd mapping:

- `0` -> active TTY input
- `1` -> active TTY output
- `2` -> active TTY output

All other fds:

- fail with a simple negative error code

Why this stage comes first:

- it defines the contract that syscalls will later expose
- it avoids hardwiring user mode directly to TTY internals
- it gives the kernel a process-like stdin/stdout model before ring 3 exists

### Stage 2: Minimal File Descriptor Table

Introduce a tiny fd table abstraction.

The first version can be very small:

```c
enum fd_kind {
    FD_KIND_NONE = 0,
    FD_KIND_TTY = 1
};

struct file_descriptor {
    enum fd_kind kind;
    void *object;
};
```

And:

```c
struct process {
    struct file_descriptor fds[3];
};
```

For the first milestone:

- only `fds[0]`, `fds[1]`, and `fds[2]` need to exist
- all three initially point to the kernel-owned TTY

This is enough to model stdin/stdout/stderr without implementing a full VFS or
open-file object system.

### Stage 3: Process Object

Introduce a minimal `struct process`.

The first version should own:

- user stack pointer
- user instruction pointer / entry point
- fd table
- maybe a saved register frame for syscall/return handling

Suggested first shape:

```c
struct process {
    struct file_descriptor fds[3];
    uintptr_t entry_point;
    uintptr_t user_stack_top;
};
```

There should also be:

- one global `current_process`

This is enough for a single-process OS.

Important: terminal mode should still live on the TTY object, not inside the
process.

## Stage 4: Syscall ABI

Introduce the first syscall entry mechanism.

Recommended first choice:

- software interrupt based syscall entry, such as `int 0x80`

Why this is the right first choice:

- simple to understand
- simple to debug
- works fine for an educational 32-bit x86 OS
- avoids extra complexity of `sysenter`/`syscall` for the first milestone

Recommended first syscall numbers:

```c
enum syscall_number {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_EXIT = 2,
    SYS_TCSETMODE = 3
};
```

Recommended first calling convention:

- `eax` = syscall number
- `ebx`, `ecx`, `edx` = first arguments
- return value in `eax`

Example:

- `read(fd, buf, len)`:
  - `eax = SYS_READ`
  - `ebx = fd`
  - `ecx = user buffer`
  - `edx = len`

- `write(fd, buf, len)`:
  - `eax = SYS_WRITE`
  - `ebx = fd`
  - `ecx = user buffer`
  - `edx = len`

The kernel syscall dispatcher should:

1. validate the syscall number
2. decode arguments
3. route to `kread` / `kwrite` / process exit / tty mode setter
4. return a simple integer result

## Stage 5: Ring 3 Transition

Introduce a minimal user-mode entry path.

The first version needs:

- GDT entries for ring 3 code and data segments
- a way to build the initial user stack
- an `iret`-based transition into ring 3

Recommended first approach:

1. kernel creates a process object
2. kernel prepares a user stack in a fixed memory region
3. kernel loads segment selectors for ring 3
4. kernel uses `iret` to enter the user entry point with the user stack

This keeps the first ring transition explicit and inspectable.

## Stage 6: User Program Placement

The first user program does not need a filesystem loader.

Recommended first approach:

- link a tiny user program blob into the kernel build
- or compile a separate user object and place it at a fixed memory location
- the kernel treats that as the initial process image

This is intentionally crude, but it is enough to prove:

- ring 3 execution works
- syscalls work
- stdin/stdout wiring works

Only after that should executable loading become a separate milestone.

## Stage 7: Userspace API

Provide a very small userspace support layer.

Minimum userspace API:

```c
int read(int fd, char *buf, int len);
int write(int fd, const char *buf, int len);
void exit(int status);
int tcsetmode(int fd, int mode);
```

Implementation:

- thin inline-asm syscall wrappers

The first userspace program should not need libc.

## Terminal Behavior In This Milestone

The current terminal stack already defines most of this behavior.

Expected first behavior:

- `read(0, ...)` reads from the active TTY
- cooked mode is the default
- cooked mode returns after newline
- backspace edits the pending cooked line
- `write(1, ...)` writes to the TTY output path
- output appears on the text console and VGA display

Raw/cooked mode changes:

- should apply to the TTY object
- may be exposed through a small `SYS_TCSETMODE`
- should fail if the fd is not TTY-backed once non-TTY fds exist

For the first single-process build, fd `0` will always be TTY-backed.

## Memory Model

For the first milestone, shared address space is acceptable.

That means:

- user and kernel live in the same linear address space
- ring protection exists
- but there is not yet per-process paging isolation

Why this is acceptable:

- it gets ring 3 and syscalls working sooner
- it keeps the milestone small
- paging-based isolation can be a later milestone

This should be documented explicitly so it is clear that "user mode" here
means privilege separation first, not full VM isolation.

## Minimal Errors

The first syscall layer does not need a full errno system.

Simple first policy:

- success returns non-negative values
- failure returns `-1` or a small negative code

That is enough for the first user programs.

## Suggested File Areas

These names are suggestions, not a mandatory final tree:

- `include/kernel_io/io.h`
- `src/kernel_io/io.c`
- `include/kernel/process.h`
- `src/kernel/process.c`
- `include/arch/x86/syscall.h`
- `src/arch/x86/syscall.c`
- `include/arch/x86/usermode.h`
- `src/arch/x86/usermode.c`
- `user/`
- `user/lib/`
- `user/apps/`

The important split is:

- generic kernel I/O and process state outside `arch/`
- x86 syscall/trap/usermode entry inside `arch/x86/`
- userspace code in its own tree

## Suggested Implementation Order

1. add `kread` / `kwrite` over the existing TTY object
2. add a minimal fd table and one `struct process`
3. route kernel code through that fd layer instead of calling TTY directly
4. add x86 syscall interrupt handling and a syscall dispatcher
5. add userspace syscall wrappers
6. add ring 3 GDT entries and `iret` entry path
7. compile and launch one fixed-address user program
8. make that program use `write(1, ...)`
9. make that program use `read(0, ...)`
10. add a minimal `tcsetmode`-style syscall if needed

This order gives a working vertical slice as early as possible.

## Testing Strategy

### Hosted Unit Tests

Continue to keep logic-heavy modules unit-tested:

- fd routing
- kernel read/write dispatch
- simple process fd initialization
- userspace syscall wrapper argument packing if convenient

### QEMU E2E Tests

Add new end-to-end tests incrementally:

1. boot and verify user program startup text appears
2. send keyboard input and verify user-space `read()` receives it
3. verify user-space `write()` reaches the text console / VGA path

The first e2e user-mode test should prove:

- ring 3 code runs
- syscall entry and return work
- stdin/stdout wiring is real

## First Demo Recommendation

The best first user-mode program is not a shell.

It should be one of:

- `hello_user`: writes a fixed line, then exits or spins
- `echo_line`: writes a prompt, reads one cooked line from stdin, writes it back

`echo_line` is the better milestone because it exercises:

- `write`
- `read`
- cooked terminal behavior
- the full input/output stack

## Open Design Choices

These need a decision during implementation, but they do not block the plan:

- exact syscall interrupt number
- exact user program load address
- whether `SYS_TCSETMODE` belongs in the first user-mode slice or the second
- whether user `exit()` should halt, spin, or return to a kernel loop in the
  single-process build

## Definition Of Done

This user-mode milestone is done when:

- the kernel can enter ring 3
- one user process runs
- that process can call `read` and `write` through syscalls
- fd `0/1/2` are wired to the current TTY
- output reaches the text console and VGA display
- keyboard input reaches the user process through the TTY stack
- a simple terminal application works in QEMU end-to-end

