# Usermode Stage 3: Ring 3 Entry And First User Process

## Scope

This document defines the final major task of the minimal usermode milestone.

Stages 1 and 2 already introduced:

- a minimal process object
- fd wiring for stdin/stdout/stderr
- `kread` / `kwrite`
- a syscall dispatcher
- x86 `int 0x80` syscall entry

This stage adds:

- the first ring 3 transition
- one built-in user process
- a tiny userspace syscall wrapper layer
- the first real user program that talks to the terminal stack

## Goal

After this stage, MademOS should be able to:

1. boot into the kernel
2. initialize the existing input/TTY/console/display stack
3. prepare one user process
4. enter ring 3 with that process
5. let the process call `read` and `write` through `int 0x80`
6. show the process output on the text console
7. deliver keyboard input from the TTY path back to the process

This completes the first true usermode vertical slice.

## Non-Goals

This stage does not include:

- multiple processes
- preemptive scheduling
- paging-based address-space isolation
- ELF loading from disk
- a shell parser
- process replacement
- fork/exec
- signals
- dynamic linking or libc

## High-Level Plan

The stack after this stage should be:

`user program -> int 0x80 -> syscall dispatcher -> kread/kwrite -> tty ->
text_console -> vga_display`

And for input:

`keyboard IRQ -> input -> tty -> kread -> syscall return -> user buffer`

## Core Design Choice

Use a fixed built-in user program in a shared address space.

That means:

- the program is compiled as part of the image
- the kernel knows its entry address ahead of time
- ring 3 executes in the same linear address space as the kernel
- privilege separation exists
- full VM isolation does not exist yet

This is the right tradeoff for the first milestone because it proves:

- ring transition
- syscall entry/return
- user/kernel split

without also introducing:

- a loader
- paging
- executable relocation

## Proposed Files

New files:

- `include/arch/x86/usermode.h`
- `src/arch/x86/usermode.c`
- `include/user/syscall.h`
- `user/lib/syscall.c`
- `user/apps/echo_line.c`

Likely existing files to change:

- `kernel.c`
- possibly `link.ld`
- possibly `bootS.asm` only if memory placement forces it

## Ring 3 Entry Model

### x86 Requirements

The kernel needs:

- ring 3 code segment
- ring 3 data segment
- a user stack pointer
- an instruction pointer for the user entry
- an `iret`-based transition

Recommended first approach:

1. add user-mode GDT entries if they are not already present
2. prepare a user stack in a fixed memory region
3. call an x86 helper that executes `iret` into ring 3

### Proposed x86 API

`include/arch/x86/usermode.h`

```c
#ifndef ARCH_X86_USERMODE_H
#define ARCH_X86_USERMODE_H

#include <stdint.h>

void x86_enter_usermode(uint32_t entry, uint32_t user_stack_top);

#endif
```

`src/arch/x86/usermode.c`

Responsibilities:

- execute the actual ring transition
- load ring 3 segment selectors
- push the correct `ss`, `esp`, `eflags`, `cs`, and `eip`
- finish with `iret`

This file should be x86-only and should not know about TTY or syscalls beyond
the fact that user code will use them later.

## Process Shape For This Stage

The existing `struct process` should grow minimally to support user entry.

Suggested new fields:

```c
struct process {
    struct file_descriptor fds[3];
    uintptr_t entry_point;
    uintptr_t user_stack_top;
};
```

Why this is enough:

- only one process exists
- the kernel only needs to know where to jump and which stack to use

No scheduler fields are needed yet.

## User Stack

Use a fixed user stack region.

Suggested first approach:

- reserve a static array in the kernel image
- point `user_stack_top` to the top of that array

Example shape:

```c
static uint8_t user_stack[4096];
```

And:

```c
process.user_stack_top = (uintptr_t)(user_stack + sizeof(user_stack));
```

This is enough for the first program.

## Built-In User Program

The first user program should be compiled into the image at build time.

Recommended program:

- `echo_line`

Behavior:

1. write a startup line, for example `"user mode ready\n"`
2. call `read(0, buf, len)`
3. write the bytes back to `stdout`
4. loop

This is better than a one-shot hello-world because it proves:

- user output path
- user input path
- syscall read/write
- cooked terminal behavior

### Proposed Userspace Entry

The first entry point can simply be:

```c
void user_main(void);
```

No CRT is needed yet.

## Userspace Syscall Wrapper Layer

Provide a tiny userspace-facing syscall wrapper library.

### Proposed Header

`include/user/syscall.h`

```c
#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

int read(int fd, char *buf, int len);
int write(int fd, const char *buf, int len);

#endif
```

### Proposed Implementation

`user/lib/syscall.c`

Responsibilities:

- expose tiny `read` and `write` wrappers
- issue `int 0x80`
- pass:
  - `eax` = syscall number
  - `ebx`, `ecx`, `edx` = args
- return result from `eax`

No libc-style extras are needed.

## Program Placement Strategy

There are two reasonable first approaches:

### Option A: Link User Code Into The Main Image

Pros:

- simplest build integration
- no separate loader

Cons:

- less explicit separation between kernel and user code

### Option B: Link User Code Separately And Place It At A Fixed Address

Pros:

- clearer separation

Cons:

- more build complexity now

Recommended choice for this milestone:

- Option A

This is the fastest way to prove the usermode path. The architecture can be
refined later.

## Kernel Wiring

`kernel.c` should:

1. initialize the current terminal stack
2. initialize the process object
3. set:
   - stdio fd wiring
   - entry point
   - user stack top
4. enable interrupts
5. enter user mode through `x86_enter_usermode(...)`

Suggested flow:

```c
process_init(&kernel_process);
process_set_tty_stdio(&kernel_process, &kernel_tty);
process_set_current(&kernel_process);
kernel_process.entry_point = (uintptr_t)user_main;
kernel_process.user_stack_top = (uintptr_t)(user_stack + sizeof(user_stack));

asm volatile("sti");
x86_enter_usermode(
    (uint32_t)kernel_process.entry_point,
    (uint32_t)kernel_process.user_stack_top
);
```

After the jump, the kernel should not continue normal execution unless the user
program traps back in a controlled way.

## What Happens If The User Program Returns

Returning from `user_main()` should not be allowed to fall through silently.

Recommended first behavior:

- user code loops forever after finishing
- or call a simple `exit()` later

For this stage, the easiest rule is:

- `user_main()` never returns

That keeps the milestone smaller.

## GDT Requirement

This stage assumes ring 3 segments exist or will be added as part of the work.

If the current GDT does not yet contain user code/data descriptors, they must
be added before `x86_enter_usermode()` can work.

This is an explicit subtask of this stage.

## EFLAGS Policy

When entering ring 3:

- interrupts should remain enabled

So the `iret` frame should preserve `IF = 1`.

That is important because the user process depends on keyboard IRQs continuing
to flow into the TTY/input path while it blocks in `read()`.

## Testing Strategy

### Hosted Unit Tests

Hosted tests are not the main value of this stage.

The logic-heavy parts are already covered:

- TTY
- input
- kernel I/O
- syscall dispatch

No hosted test is required for the actual ring transition.

### QEMU E2E Tests

This stage should extend `tests/e2e.py` with a new usermode-focused check.

Recommended checks:

1. boot and assert user-mode startup text appears
2. send a normal key sequence and Enter
3. assert the echoed or echoed-back user program output appears

For example:

- user writes: `"user mode ready\n"`
- send: `"abc"` + Enter
- assert screen contains the line returned by the user app

That proves:

- ring 3 code executed
- syscalls worked
- stdin/stdout routing worked

## Suggested Implementation Order

1. add `entry_point` and `user_stack_top` to `struct process`
2. add `include/arch/x86/usermode.h`
3. add `src/arch/x86/usermode.c`
4. ensure user GDT entries exist
5. add `include/user/syscall.h`
6. add `user/lib/syscall.c`
7. add `user/apps/echo_line.c`
8. wire kernel process entry and stack
9. call `x86_enter_usermode(...)` from `kernel.c`
10. update e2e tests

## Definition Of Done

This stage is done when:

- the kernel can enter ring 3
- one built-in user program runs
- that program can call `read` and `write`
- its output reaches the text console and VGA display
- its input comes from the existing TTY path
- QEMU e2e proves the full usermode terminal path works

