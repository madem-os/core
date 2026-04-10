# Project Layout Design

## Goal

Define a cleaner source tree for MademOS so code is grouped by responsibility:

- boot code
- architecture-specific x86 code
- drivers
- console / output code
- kernel entry / orchestration
- tests

This document is only about project structure and naming.

It is **not** an implementation task by itself.

## Current Problem

The codebase has started to grow beyond a single-file demo.

We now have:

- bootloader code
- low-level port I/O
- IDT / PIC / IRQ logic
- keyboard driver code
- console / VGA output logic
- command-mode experimentation

Some of these pieces are already split into modules, but the layout does not yet clearly express ownership.

Example:

- `mos-ports` is really x86-specific architecture code, not a generic MademOS subsystem
- `mos-stdio` is currently closer to a VGA console helper than a real stdio layer

## Design Principles

The layout should:

- make x86-specific code obvious
- separate drivers from architecture code
- keep the kernel entry point simple
- avoid naming that implies abstractions that do not exist yet
- leave room for future growth without overengineering

In addition, every source and header file in the cleaned-up tree should start with a descriptive comment header.

## File Header Requirement

Every `.c`, `.h`, and assembly source file in the clean tree should begin with a file-level comment block that explains the file's purpose.

The header should be:

- about `10` to `40` lines long
- descriptive rather than decorative
- focused on ownership and purpose
- clear about what the file should and should not contain

The goal is that someone opening the file for the first time can immediately understand:

- what subsystem it belongs to
- what responsibilities it owns
- what responsibilities belong elsewhere
- how it fits into the boot or kernel flow

### What The Header Should Cover

Each file header should usually include:

- the subsystem name
- the role of the file
- the main responsibilities of the file
- important boundaries with neighboring modules
- any key assumptions relevant to that file

### Example Expectations

Examples:

- `arch/x86/ports.c` should explain that it owns raw x86 port I/O primitives and should not contain PIC or keyboard policy
- `arch/x86/irq.c` should explain that it owns IRQ stub registration and dispatch, and that device drivers should not send EOI directly
- `drivers/keyboard.c` should explain that it owns keyboard IRQ registration and keyboard-specific logic, but not IDT or PIC setup
- `console/console.c` should explain that it owns VGA text console behavior, not true stdio semantics
- `kernel.c` should explain that it is the high-level kernel entry / orchestration file, not the home for low-level subsystem implementation

### Why This Rule Matters

This project is still small, but it is already crossing the threshold where structure matters more than raw file count.

Requiring comment headers will help:

- preserve module boundaries
- make future refactors safer
- keep architecture code, driver code, and console code from bleeding together again
- make the codebase much easier to revisit after time away from the project

## Recommended Direction

There are two good versions of the layout:

- a near-term low-churn layout
- a more polished longer-term layout

## Near-Term Layout

This version stays close to the current repository and minimizes churn:

- `bootS.asm`
- `kernel.c`
- `link.ld`
- `src/arch/x86/ports.c`
- `include/arch/x86/ports.h`
- `src/arch/x86/idt.c`
- `include/arch/x86/idt.h`
- `src/arch/x86/pic.c`
- `include/arch/x86/pic.h`
- `src/arch/x86/irq.c`
- `include/arch/x86/irq.h`
- `src/drivers/keyboard.c`
- `include/drivers/keyboard.h`
- `src/console/console.c`
- `include/console/console.h`
- `tests/`
- `tasks/`
- `bin/`

### Why This Is Good

- low disruption from the current tree
- architecture-specific code is grouped under `arch/x86`
- device code is grouped under `drivers`
- console code is no longer pretending to be stdio
- `kernel.c` remains the main entry / init file
- each file can carry a clear ownership header from the start

## Longer-Term Layout

If the project grows further, a more kernel-like layout becomes reasonable:

- `boot/bootS.asm`
- `kernel/kernel.c`
- `kernel/arch/x86/ports.c`
- `kernel/arch/x86/ports.h`
- `kernel/arch/x86/idt.c`
- `kernel/arch/x86/idt.h`
- `kernel/arch/x86/pic.c`
- `kernel/arch/x86/pic.h`
- `kernel/arch/x86/irq.c`
- `kernel/arch/x86/irq.h`
- `kernel/drivers/keyboard.c`
- `kernel/drivers/keyboard.h`
- `kernel/console/console.c`
- `kernel/console/console.h`
- `kernel/stdio/stdio.c`
- `kernel/stdio/stdio.h`
- `link/link.ld`
- `tests/`
- `tasks/`
- `bin/`

### Why This Is Good

- clearer separation between boot, kernel, and link artifacts
- better fit for a larger hobby kernel
- closer to common OS-style directory organization
- easier to keep subsystem-level comment headers consistent

## Naming Recommendations

### 1. Move `mos-ports` under `arch/x86`

Recommended rename:

- `mos-ports.c` -> `src/arch/x86/ports.c`
- `mos-ports.h` -> `include/arch/x86/ports.h`

Reason:

- port I/O is x86-specific
- `inb` / `outb` / `outw` style helpers are architecture primitives
- they are not generic kernel services

### 2. Rename `mos-stdio` to `console`

Recommended rename:

- `mos-stdio.c` -> `src/console/console.c`
- `mos-stdio.h` -> `include/console/console.h`

Reason:

- the current module is not really stdio
- it owns VGA text output behavior
- it also contains temporary console/input demo behavior
- `console` is a more accurate name for what exists today

### 3. Keep `kernel.c` as the top-level entry point

`kernel.c` should remain the high-level initialization file that:

- enters C from assembly
- initializes subsystems
- starts the main kernel loop

It should not become a dumping ground for low-level helper code.

## Why `src/` And `include/` Are Reasonable

For C projects, this is common and standard:

- `src/` for implementation files
- `include/` for shared or exported headers
- `tests/` for tests
- `bin/` or `build/` for generated output

For kernel-like and low-level systems code, another standard pattern is:

- `arch/<arch>/` for architecture-specific code
- `drivers/` for hardware drivers
- `kernel/` for core kernel logic

So for this project, using:

- `src/arch/x86`
- `src/drivers`
- `src/console`

is fully aligned with normal industry and hobby-kernel practice.

## Header Placement Guidance

There are two reasonable header strategies:

### Strategy A: Shared headers under `include/`

Use:

- `include/arch/x86/...`
- `include/drivers/...`
- `include/console/...`

This is simple and works well for the current project size.

### Strategy B: Public headers in `include/`, private headers near sources

For a larger project:

- keep public/shared headers under `include/`
- keep private module-local headers next to `.c` files in `src/`

That split is common once the codebase gets larger.

For now, Strategy A is enough.

## Recommended Migration Order

To avoid unnecessary churn, migrate in this order:

1. move `mos-ports` into `arch/x86/ports`
2. update includes to use `arch/x86/ports.h`
3. rename `mos-stdio` to `console`
4. update includes to use `console/console.h`
5. remove or replace any remaining command-mode proof-of-concept code
6. keep building future IRQ / keyboard / console work on top of the cleaner layout

## Follow-Up Documentation Task

When the layout cleanup is actually implemented, the executor should also update `README.MD`.

That README update should happen as part of the implementation task, not as a separate unrelated documentation pass.

The README update should include:

- the new current milestone summary
- the cleaned-up repository structure
- the role of `arch/x86`, `drivers`, `console`, and `kernel.c`
- the intended meaning of `src/` and `include/`
- the rule that each source and header file should begin with a descriptive file-level comment header

The purpose of this task is to keep the README aligned with the real code layout after the cleanup lands, rather than documenting the target layout too early.

## What Should Not Be Done Yet

Do not introduce a real `stdio` module name until the project actually has stdio-like behavior, for example:

- `stdin`
- `stdout`
- `stderr`
- stream-like read/write APIs
- line-buffered console reads

Until then, `console` is a more truthful name.

## Recommended Final Decision

For the next cleanup, the best balance of clarity and low churn is:

- keep `kernel.c` at the root
- move port helpers to `arch/x86/ports`
- keep IDT / PIC / IRQ under `arch/x86`
- keep keyboard under `drivers`
- rename `mos-stdio` to `console`
- continue using `src/` and `include/`
- require a descriptive file header in every source / header file in the cleaned-up tree

This gives a structure that is:

- cleaner than the current tree
- aligned with standard C / kernel project conventions
- simple enough for the current project size
- flexible enough for the next milestones
