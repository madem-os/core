# Usermode Stage 4.2: Higher-Half Kernel Migration

## Scope

This document defines the second sub-stage of the virtual-memory milestone:

- the kernel remains physically loaded at `0x00100000`
- paging is already enabled from stage 4.1
- the next step is to execute the kernel from a high virtual address

This stage does **not** yet introduce:

- per-process page directories
- isolated user mappings
- syscall pointer validation
- a separate user image

The goal here is only to migrate the current kernel image to a higher-half
virtual layout while preserving the existing behavior and test coverage.

## Current State

The current codebase has these relevant properties:

### Link Layout

The kernel is currently linked at low virtual addresses:

- [link.ld](/Users/yotammadem/mademos/core/link.ld) starts the image at
  `0x00100000`

Current symbols in `bin/kernel.elf` confirm that:

- `kernel_entry` is at `0x00100000`
- `kmain` is at `0x00100020`
- the built-in user app `user_main` is also in the same image and currently
  lives at a low linked address

### Boot Flow

The bootloader in [bootS.asm](/Users/yotammadem/mademos/core/bootS.asm):

- loads the kernel physically at `0x00100000`
- enters protected mode
- jumps directly to `0x00100000`

So today physical load address and linked execution address are the same.

### Paging State

Stage 4.1 introduced:

- [paging.c](/Users/yotammadem/mademos/core/src/arch/x86/paging.c)
- [paging.S](/Users/yotammadem/mademos/core/src/arch/x86/paging.S)

The current paging setup:

- identity-maps the first `16 MiB`
- keeps all current virtual addresses unchanged
- marks those pages user-accessible because the current ring-3 app still
  executes from the kernel-linked low image

### Early Entry Assumptions

Several places still assume low runtime addresses:

- [entry.S](/Users/yotammadem/mademos/core/src/arch/x86/entry.S)
  - initial kernel stack at `0x00090000`
- [bootS.asm](/Users/yotammadem/mademos/core/bootS.asm)
  - TSS `esp0 = 0x00090000`
  - final jump to low kernel entry
- [vga_display.c](/Users/yotammadem/mademos/core/src/arch/x86/drivers/vga_display.c)
  - VGA text buffer at low physical/identity-mapped `0x000B8000`

These low physical assumptions are acceptable during this stage as long as the
required pages stay mapped.

## Goal

After this stage, MademOS should:

1. keep loading the kernel physically at `0x00100000`
2. map the kernel image into a higher-half virtual region
3. transfer execution from the low alias to the higher-half alias
4. continue running the current kernel and ring-3 demo app successfully
5. keep unit tests and e2e tests green

This stage is about architecture, not protection.

## Non-Goals

This stage does not yet aim to:

- make the kernel region supervisor-only
- isolate user memory
- remove the low identity alias completely
- split the user app into a separate image
- introduce process-specific CR3 switching

Those belong to later steps.

## Core Design Choice

Map the existing kernel image into the higher half while temporarily keeping
the current low identity alias.

That means the image will have **two valid virtual aliases** during this
stage:

- low identity alias
- higher-half alias

Execution should migrate to the higher-half alias as early as practical after
paging is enabled.

This is the simplest safe way to:

- keep bring-up debuggable
- preserve current boot behavior
- avoid needing a fully separate user-image layout yet

## Important Transitional Constraint

The current `user_main` is linked into the same image as the kernel.

That matters because once the image is relinked into the higher half:

- `kernel_entry`, `kmain`, and all kernel symbols move high
- `user_main` also moves high, because it is still part of the same linked
  image

Therefore, if stage 4.2 still wants the current ring-3 program to work
unchanged, then the mapped image pages that contain `user_main` must remain
user-accessible for now.

This is a temporary compromise and should be stated explicitly:

- stage 4.2 improves virtual layout
- stage 4.2 does **not** yet improve user/kernel protection

Protection comes later, when the user app is moved into a dedicated user
mapping and the kernel region can become supervisor-only.

## Recommended Virtual Layout

Use these conceptual constants:

```c
KERNEL_PHYS_BASE = 0x00100000
KERNEL_VIRT_BASE = 0xC0000000
KERNEL_LINK_BASE = KERNEL_VIRT_BASE + KERNEL_PHYS_BASE
```

That gives:

- physical kernel load: `0x00100000`
- linked virtual kernel base: `0xC0100000`

This is a conventional higher-half layout and keeps the physical load address
unchanged.

## Linker Design

The linker script should be changed so the kernel is **linked** at
`0xC0100000`, but still **loaded** from physical `0x00100000`.

That means [link.ld](/Users/yotammadem/mademos/core/link.ld) will need a real
split between:

- virtual memory address (VMA)
- load memory address (LMA)

The important outcome is:

- symbol addresses seen by the compiler and generated code are high-half VAs
- the bootloader still copies the image to the same low physical address

The design should use linker-defined symbols to make the physical/virtual
relationship explicit, for example:

```ld
KERNEL_PHYS_BASE = 0x00100000;
KERNEL_VIRT_BASE = 0xC0000000;

. = KERNEL_PHYS_BASE + KERNEL_VIRT_BASE;

.text : AT(ADDR(.text) - KERNEL_VIRT_BASE) { ... }
```

The exact linker syntax can vary, but the key idea is:

- high VMA
- low LMA

## Paging Design For This Stage

The current identity-only paging setup in
[paging.c](/Users/yotammadem/mademos/core/src/arch/x86/paging.c) should be
extended, not replaced.

### Required Mappings

During this stage the bootstrap page tables should provide:

1. low identity mapping for the currently executing bootstrap/kernel region
2. higher-half mapping for the same physical kernel image pages
3. enough low identity-mapped memory to preserve:
   - VGA text buffer access at `0x000B8000`
   - current low stack/TSS assumptions
   - early data structures still living low

### Page Permissions

For this stage, the mapped kernel image pages should remain:

- present
- writable where needed
- user-accessible

That is not the desired final protection model, but it is the simplest way to
keep the current built-in ring-3 app functional after the image moves high.

Later stages should change this by:

- placing user code into a dedicated low user region
- making the high kernel region supervisor-only

## Transition Flow

The migration should happen in this order:

1. bootloader loads the kernel physically to `0x00100000`
2. bootloader still jumps to the low physical entry address
3. low entry code reaches C as today
4. paging setup installs:
   - low identity mappings
   - high-half alias mappings
5. paging is enabled
6. execution jumps to the higher-half kernel address
7. the rest of the kernel continues from the high alias

The jump in step 6 should be explicit and intentional. Do not rely on falling
into the higher half implicitly.

## Where The High-Half Jump Should Happen

There are two reasonable options.

### Option A: Jump In Boot Assembly

Modify [bootS.asm](/Users/yotammadem/mademos/core/bootS.asm) so paging is
enabled there and the first jump into the kernel is already to the high-half
entry.

Pros:

- very explicit control over the transition
- keeps early low/high alias logic in one place

Cons:

- more assembly complexity in the bootloader
- more coupling between boot code and kernel paging layout

### Option B: Jump In The Kernel Paging Bring-Up

Keep boot behavior mostly unchanged, but teach the paging module/kernel entry
path to jump from low execution to the high-half `kmain` / `kernel_entry`
continuation once both aliases exist.

Pros:

- keeps paging policy closer to the kernel
- reduces bootloader churn

Cons:

- requires a carefully designed trampoline point
- stack and current instruction address assumptions must be handled cleanly

Recommendation:

- prefer **Option B** if it can be kept small and explicit
- use boot assembly only for the minimum required physical load + protected
  mode entry

That keeps higher-half layout policy in the kernel-side code instead of
spreading it further into the bootloader.

## Early Stack And TSS

Current low stack assumptions:

- [entry.S](/Users/yotammadem/mademos/core/src/arch/x86/entry.S) uses
  `0x00090000`
- [bootS.asm](/Users/yotammadem/mademos/core/bootS.asm) sets TSS `esp0` to
  `0x00090000`

For this stage, those may remain physically low and identity-mapped.

That means the first higher-half migration does **not** need to move:

- the bootstrap kernel stack
- the TSS kernel stack pointer

Later cleanup can move these into proper higher-half-managed kernel memory if
desired.

## Low Alias Policy

Do not remove the low alias in this stage.

Keep it intentionally for:

- safe bring-up
- VGA and other legacy low-memory assumptions
- temporary support for code paths that still depend on low-mapped objects

The right rule for stage 4.2 is:

- higher-half execution becomes the normal path
- low alias remains as a compatibility mapping

Low alias removal can be a later cleanup after:

- user image is separated
- kernel region permissions are tightened
- low-memory dependencies are intentionally reduced

## Affected Modules

### `link.ld`

Must change to:

- link the kernel at a higher-half VMA
- preserve low physical load addresses through LMAs
- export any needed symbols for the paging transition

### `bootS.asm`

Will likely need at least small changes for:

- awareness of the new high linked entry symbol or transition point
- preserving the physical jump/load model while the linked addresses go high

It should still avoid absorbing paging policy that belongs in the kernel.

### `src/arch/x86/paging.c`

Must evolve from:

- identity mapping only

to:

- identity mapping plus higher-half alias mapping

It should also own any explicit higher-half transition helper if that path is
implemented on the kernel side.

### `src/arch/x86/entry.S`

May need updates depending on where the higher-half jump occurs.

If execution reaches `kernel_entry` directly at its higher-half VA, this file
may need little or no change beyond continuing to use the current stack.

### `kernel.c`

May need a small adjustment if there is an explicit “continue in high-half”
step before normal subsystem initialization.

### `src/arch/x86/drivers/vga_display.c`

No architectural change is required yet.

It may continue using:

- `0x000B8000`

as long as that low page remains mapped in the bootstrap paging model.

## Proposed Files

Likely files to change:

- [link.ld](/Users/yotammadem/mademos/core/link.ld)
- [bootS.asm](/Users/yotammadem/mademos/core/bootS.asm)
- [paging.c](/Users/yotammadem/mademos/core/src/arch/x86/paging.c)
- [paging.h](/Users/yotammadem/mademos/core/include/arch/x86/paging.h)
- possibly [entry.S](/Users/yotammadem/mademos/core/src/arch/x86/entry.S)
- possibly [kernel.c](/Users/yotammadem/mademos/core/kernel.c)

Possibly new file if the transition wants a dedicated helper:

- `src/arch/x86/higher_half.S`

This is optional. The main design point is clarity, not file count.

## Recommended Implementation Order

1. update linker script for high VMA / low LMA
2. extend paging setup to map both low identity and high-half kernel aliases
3. introduce an explicit jump/continuation into the higher-half kernel address
4. keep the low alias alive
5. run:
   - `./build.sh --build-only`
   - `./tests/run-unit.sh`
   - `python3 tests/e2e.py`
6. only after the system is stable, consider tightening any assumptions

## Test Plan

### Hosted Tests

Hosted unit tests will not directly validate the higher-half transition. They
mainly guard unrelated regressions in:

- TTY
- input
- kernel I/O
- syscall dispatch
- console behavior

That is still useful, because the migration must not break those layers.

### Freestanding / E2E Verification

The critical checks are in QEMU:

1. boot still succeeds
2. `welcome to Madem-OS!` appears
3. `user mode ready` appears
4. keyboard input still reaches the user program

The existing e2e suite already checks exactly that, which makes this stage a
good fit for incremental migration.

## Definition Of Done

This stage is complete when:

- the kernel is linked high and executed from the higher half
- the kernel is still physically loaded at `0x00100000`
- the low alias still exists during bring-up
- the current ring-3 app still works unchanged
- existing unit tests pass
- existing e2e tests pass

At that point, the architecture will be ready for the next step:

- keep the kernel high and shared
- give each process its own low user mappings
- move the user app out of the temporary “kernel image is user-accessible”
  compromise
