# Keyboard IRQ Registration Design

## Scope

This document covers only the next task:

- register the keyboard interrupt handler cleanly

It does **not** cover:

- ring buffers
- line editing
- console / TTY behavior
- `stdin` / `stdout` abstractions
- shell behavior

Those come later.

## Problem

The current interrupt code in `src/mos-cmd.c` is a proof of concept.

It mixes together:

- IDT setup
- PIC remapping
- IRQ1 handler stub
- keyboard interrupt handling
- command-mode experimentation

That is enough to prove interrupts work, but it is not a clean foundation for a real keyboard driver.

## Goal

Create a clean interrupt registration path so the keyboard driver can register IRQ1 without owning IDT or PIC setup details.

The target is:

1. architecture code sets up IDT and PIC
2. a generic IRQ layer dispatches hardware IRQs
3. the keyboard driver registers a handler for IRQ1
4. the keyboard handler only handles keyboard-specific work

## Non-Goals

This task should not:

- add cooked terminal behavior
- add command parsing
- print directly from IRQ code
- add a generic device framework
- introduce syscalls

## Design Overview

Split responsibilities into four modules:

- `idt`
- `pic`
- `irq`
- `keyboard`

### 1. IDT Module

Suggested files:

- `src/arch/x86/idt.c`
- `include/arch/x86/idt.h`

Responsibilities:

- own the IDT table
- define IDT entry structures
- expose `idt_init()`
- expose `idt_set_gate(vector, handler)`
- load the IDT with `lidt`

The IDT module should know nothing about the keyboard.

### 2. PIC Module

Suggested files:

- `src/arch/x86/pic.c`
- `include/arch/x86/pic.h`

Responsibilities:

- remap PIC interrupts
- mask and unmask IRQ lines
- send EOI
- keep `io_wait()` usage local to PIC programming

Suggested API:

- `void pic_init(void);`
- `void pic_set_mask(uint8_t irq);`
- `void pic_clear_mask(uint8_t irq);`
- `void pic_send_eoi(uint8_t irq);`

The PIC module should know nothing about keyboard scancodes or console output.

### 3. IRQ Module

Suggested files:

- `src/arch/x86/irq.c`
- `include/arch/x86/irq.h`

Responsibilities:

- provide assembly stubs for IRQ vectors
- install those stubs into the IDT
- own a handler table for IRQ0..IRQ15
- dispatch IRQs to registered C handlers
- send EOI after handler execution

Suggested handler type:

```c
typedef void (*irq_handler_t)(void);
```

Suggested API:

- `void irq_init(void);`
- `void irq_install_handler(uint8_t irq, irq_handler_t handler);`
- `void irq_uninstall_handler(uint8_t irq);`

Suggested IRQ numbers:

- IRQ0..IRQ15 as logical PIC IRQ numbers
- mapped vectors start at `0x20`

Example:

- IRQ1 = keyboard
- vector = `0x21`

The IRQ module should not know keyboard protocol details.

### 4. Keyboard Module

Suggested files:

- `src/drivers/keyboard.c`
- `include/drivers/keyboard.h`

Responsibilities for this task only:

- register the keyboard IRQ handler
- read scancode from port `0x60`
- keep keyboard-specific logic in the keyboard module

Suggested API:

- `void keyboard_init(void);`

For this task, the keyboard handler can remain minimal and temporary. The main point is that it must be registered through the generic IRQ layer rather than wired directly in a command-mode experiment.

## Initialization Flow

The kernel interrupt initialization flow should become:

1. `idt_init()`
2. `pic_init()`
3. `irq_init()`
4. `keyboard_init()`
5. enable interrupts with `sti`

Conceptually:

```c
void kernel_init_interrupts(void) {
    idt_init();
    pic_init();
    irq_init();
    keyboard_init();
    asm volatile("sti");
}
```

## Keyboard Registration Flow

Inside `keyboard_init()`:

1. register the keyboard IRQ handler for IRQ1
2. unmask IRQ1 in the PIC

Conceptually:

```c
void keyboard_init(void) {
    irq_install_handler(1, keyboard_irq_handler);
    pic_clear_mask(1);
}
```

## IRQ Dispatch Flow

The runtime flow should be:

1. hardware raises IRQ1
2. CPU enters the IRQ1 stub
3. stub saves registers and calls the common IRQ dispatcher
4. dispatcher maps IRQ1 to the installed handler
5. keyboard handler runs
6. dispatcher sends EOI through the PIC module
7. return from interrupt

## Important Design Rules

### Rule 1: EOI belongs in the IRQ/PIC framework

The keyboard driver should not send EOI directly.

Reason:

- EOI is interrupt-controller policy
- it should stay centralized
- all IRQ users should follow one model

### Rule 2: Keyboard driver should not own IDT/PIC setup

The keyboard driver should not:

- set IDT gates directly
- remap the PIC
- define architecture-wide interrupt structures

Reason:

- those are architecture responsibilities, not device-driver responsibilities

### Rule 3: Keep the keyboard IRQ handler small

For this task, the keyboard handler should do as little as possible.

At minimum:

- read port `0x60`
- store or forward the scancode in keyboard-owned code

It should not:

- print directly to VGA as a long-term design
- perform command logic
- perform terminal editing

### Rule 4: Keep this task narrowly scoped

This task is only about clean registration and dispatch.

Do not block it on:

- ring buffers
- shell code
- TTY semantics
- stream APIs

## Suggested File Ownership

### `idt`

- data structures for IDT entries
- `idt_init`
- `idt_set_gate`
- `lidt`

### `pic`

- PIC remap
- IRQ masking / unmasking
- EOI handling
- PIC-specific waits

### `irq`

- IRQ stubs
- dispatch table
- C-level registration API

### `keyboard`

- IRQ1 registration
- keyboard IRQ handler
- keyboard-specific state

## Minimal Success Criteria

This task is done when:

- no keyboard-specific setup remains in the current interrupt POC
- the IDT is managed by an IDT module
- PIC logic is managed by a PIC module
- IRQ handlers are registered through a generic IRQ API
- the keyboard driver registers IRQ1 through that API
- the keyboard interrupt handler is called successfully after `sti`

## E2E Test Strategy For This Task

This task should have a basic end-to-end test using the existing testing approach in the repo:

- build the image
- boot QEMU headlessly
- interact with QEMU through the monitor
- inspect VGA memory

The goal of this test is **not** to validate the final keyboard subsystem design.

It is only meant to prove that:

- IDT setup works
- PIC remap / unmask works
- IRQ1 is registered correctly
- the keyboard IRQ handler is actually invoked

### Proposed Test Contract

For this milestone, the kernel should expose one simple observable behavior:

1. boot and print a known startup string
2. enable interrupts and register IRQ1
3. when one key is pressed, produce one deterministic visible effect on the VGA console

Example:

- initial text: `interrupts ready`
- after injecting key `a`: text becomes `interrupts readya`

This is enough to prove that the interrupt registration path works end-to-end.

### How To Implement It With Current Infrastructure

Add a second Python e2e test, similar to the existing VGA test.

Suggested file:

- `tests/e2e_keyboard_irq.py`

Suggested flow:

1. run `./build.sh --build-only`
2. launch QEMU with:
   - `-display none`
   - `-monitor stdio`
   - `-serial none`
3. wait briefly for boot to complete
4. send a monitor command such as:
   - `sendkey a`
5. dump VGA memory:
   - `xp /160bx 0xb8000`
6. decode the VGA characters
7. assert that the expected visible change occurred

### Why Use QEMU `sendkey`

Using `sendkey` is a good fit here because:

- it works with the existing monitor-driven test approach
- it avoids building a new host-side keyboard injection mechanism
- it tests the real IRQ path instead of directly calling keyboard code

### What This Test Should Assert

At this milestone, assert only the narrow contract:

- keyboard IRQ registration is live
- one keypress reaches the registered keyboard handler
- the handler's effect becomes visible in VGA memory

Do **not** require this test to validate:

- full scancode translation correctness
- Shift behavior
- Backspace behavior
- line buffering
- shell interaction

Those belong to later milestones.

### Recommended Temporary Behavior

Until the full keyboard driver and buffer are implemented, it is acceptable for the keyboard IRQ path to have a very small, deterministic visible effect purely for testability.

For example:

- echo one printable key directly
- print a marker character when IRQ1 fires

This is acceptable as a temporary milestone behavior, as long as it is removed or replaced once the real keyboard buffering layer is added.

## Out Of Scope Follow-Up

After this task is complete, the next logical task is:

- move from direct IRQ-side handling to keyboard buffering

That is the point where the ring buffer work from the larger roadmap should begin.
