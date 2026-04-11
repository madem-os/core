# Usermode Stage 4.3: First Isolated User Mapping

## Scope

This document defines the next stage after the higher-half kernel migration.

The current system now has:

- paging enabled
- the kernel linked and executing in the higher half
- the kernel physically loaded low
- the current ring-3 user program still working

But it still has one major transitional limitation:

- the built-in user program is linked into the same image as the kernel
- therefore the higher-half kernel image is still mapped user-accessible

This stage removes that limitation.

The goal is to create the first honest isolated user address space while still
keeping the system simple:

- one process
- one address space
- one built-in user program
- no scheduler
- no `spawn/exec` yet

## Goal

After this stage, MademOS should be able to:

1. keep the kernel mapped high and shared
2. create one process-owned low user virtual mapping
3. copy the built-in user program into that user mapping
4. enter ring 3 at the user virtual entry point
5. keep stdin/stdout working through the existing syscall/TTY path
6. make the higher-half kernel mapping supervisor-only

This is the first stage that should deliver real memory isolation in practice.

## Non-Goals

This stage does not yet include:

- multiple processes
- scheduler/context switching
- `spawn`, `exec`, `fork`, or `wait`
- loading executables from disk
- full page-fault handling
- dynamic user heap allocation
- demand paging

Those come later.

## Testability Requirement

This stage must be implemented in a testable way.

Specifically:

- privileged x86 operations must stay in the x86-specific layer
- VM layout and mapping policy must live in plain C modules that can be tested
  in hosted unit tests
- new VM logic should not be written as one monolithic paging routine tightly
  coupled to `mov %cr3`, `mov %cr0`, or ring-transition assembly

The current bootstrap paging refactor is the model to follow:

- page-table construction policy is testable in hosted mode
- the actual CPU register manipulation stays in the x86 assembly layer

Stage 4.3 should follow the same split for:

- user-region layout calculation
- page count calculation
- stack + guard page placement
- user image placement and copy boundaries
- process `vm_space` initialization

## Why This Stage Comes Next

After the higher-half migration, the architecture is cleaner, but protection is
still incomplete.

Right now:

- the kernel is high
- but the image containing both kernel code and `user_main` must remain
  user-accessible

That means a ring-3 program can still legally touch kernel-image pages.

So the next step is not merely “add a page directory field.” The next step is:

- give the process a real user mapping
- move the built-in user program into it
- then tighten kernel permissions

That is the first point where the system becomes conceptually honest about user
vs kernel memory.

## High-Level Design

The intended model after this stage is:

- kernel virtual region:
  - high-half
  - present
  - supervisor-only
  - identical across all processes

- user virtual region:
  - low addresses
  - present
  - user-accessible
  - process-owned

Conceptually:

`user process -> low user VA pages`

and:

`kernel / syscalls / interrupts -> shared high kernel VA pages`

The currently built-in user app should no longer execute from the kernel image.
Instead:

- the kernel copies the user image into mapped user pages
- `process.entry_point` becomes the user virtual entry
- `process.user_stack_top` becomes the user virtual stack top

## Fixed User Virtual Contract

Use a fixed user window for this stage.

Recommended constants:

```c
USER_BASE      = 0x00400000
USER_TEXT_BASE = 0x00400000
USER_HEAP_BASE = 0x00800000   /* reserved for later */
USER_STACK_TOP = 0x00C00000
USER_LIMIT     = 0x00C00000
```

Important rules:

- the user window is a fixed contract
- the kernel must not place its own runtime objects there
- the initial user program and stack must fit inside that range

## Process VM Shape

This stage should introduce an explicit VM descriptor on the process.

Suggested shape:

```c
struct vm_space {
    uintptr_t user_base;
    uintptr_t user_limit;
    uintptr_t text_base;
    size_t text_size;
    uintptr_t stack_top;
    size_t stack_size;
    struct page_directory *page_directory;
};

struct process {
    struct file_descriptor fds[3];
    uintptr_t entry_point;
    uintptr_t user_stack_top;
    struct vm_space vm_space;
};
```

Why this is useful now:

- syscall validation will later need these bounds
- the process owns its address-space contract explicitly
- it avoids scattering address-space constants across unrelated modules

## Kernel Mapping Policy

After this stage, the higher-half kernel mapping should be:

- present
- writable where needed
- supervisor-only

That applies to:

- kernel text/data/bss
- kernel stacks
- syscall code
- IRQ/IDT/GDT/TSS related runtime memory
- shared kernel services used on behalf of the process

The kernel region remains shared across all future processes.

## User Mapping Policy

The first process should receive:

1. user text/data pages at `USER_TEXT_BASE`
2. user stack pages ending at `USER_STACK_TOP`
3. one unmapped guard page below the stack

For this first step:

- text may remain writable if that simplifies bring-up
- stack should be writable
- all user pages must carry the user-accessible bit

## Built-In User Image Strategy

The current built-in user app should still be compiled into the image, but it
should no longer execute in place.

Recommended strategy:

1. dedicate a user-image section in the linked kernel image
2. export linker symbols for:
   - user image start
   - user image end
   - user entry offset or entry symbol
3. during process VM setup:
   - allocate/map user text pages
   - copy the image bytes from the kernel image into `USER_TEXT_BASE`
4. set:
   - `process.entry_point` to the copied user virtual entry
   - `process.user_stack_top` to `USER_STACK_TOP`

This keeps the current build model simple while fixing the runtime placement.

### Important Principle

Do not keep using the high-half linked `user_main` address as the ring-3 entry.

That would preserve the current compromise and defeat the purpose of this
stage.

## Page Directory Model

This stage still uses one process, but it should do so through a genuine
process-owned page directory.

The intended model is:

- one shared kernel mapping region copied/reused into the process page
  directory
- one user mapping region populated specifically for that process

The kernel should continue running while the current process CR3 is active.

That means:

- syscalls and interrupts execute with:
  - shared high kernel mappings
  - current process low user mappings

This is the same CR3 model intended for future multi-process work.

## Required VM Operations

This stage should introduce enough VM support to do the following:

1. create a fresh page directory for the process
2. install the shared higher-half kernel mappings into it
3. allocate physical pages for user text
4. allocate physical pages for user stack
5. map those pages into the user window
6. copy the user image into mapped user text pages
7. switch CR3 to the process page directory before entering ring 3

This is enough for one isolated process without requiring a general-purpose VM
manager yet.

## Frame Allocation

The page-frame allocator can still be a simple bump allocator in this stage,
but it must follow explicit reserved-memory rules.

It must not allocate over:

- kernel image
- bootstrap stack
- bootstrap paging structures
- user image blob embedded in the kernel image
- VGA/device low-memory windows
- allocator metadata itself

The allocator only needs to support page-sized allocations for now.

## Syscall Behavior During This Stage

This stage does not yet require full user-pointer validation, but it should be
implemented with the next stage in mind.

That means:

- `read` / `write` continue to work for the copied user program
- later validation logic should use `process.vm_space`

If the implementation naturally makes minimal validation easy here, that is
fine, but it is not the primary goal of this stage.

## Proposed Files

Likely files to add:

- `include/kernel/vm.h`
- `src/kernel/vm.c`

Likely files to change:

- `include/kernel/process.h`
- `src/kernel/process.c`
- `kernel.c`
- `link.ld`
- `user/apps/...` only if user image section placement needs annotations
- `src/arch/x86/paging.c`
- `src/arch/x86/paging.S` if extra CR3 helpers are needed
- `src/arch/x86/usermode_stub.S` only if entry assumptions need small cleanup

Possible linker symbols to introduce:

- `_user_image_start`
- `_user_image_end`
- `_user_image_entry`

Exact symbol names can vary; the important thing is to make the copy boundary
explicit.

## Kernel Initialization Flow

The high-level startup sequence after this stage should be:

1. early boot enters the higher-half kernel
2. kernel initializes interrupts, TTY, console, etc.
3. kernel initializes one process object
4. kernel wires stdio to the active TTY
5. kernel initializes the process VM space
6. kernel creates/maps the user text and stack pages
7. kernel copies the built-in user image into user memory
8. kernel sets user entry and stack pointers to user virtual addresses
9. kernel switches to the process page directory
10. kernel enters ring 3 at the user virtual entry

## Hosted Test Plan

Hosted tests should focus on VM policy, not privileged paging instructions.

Suggested hosted tests:

- `vm_space` initialization
- user-range constants / helper behavior
- image-size-to-page-count calculation
- stack layout calculation including guard page
- process page-directory policy setup where the kernel mapping region is copied
  or reused in the expected slots
- user text mapping layout for the built-in user image
- user stack mapping layout including the unmapped guard page

If the `kernel/vm` code is written to separate policy from x86 instruction
helpers, most of this can be tested in hosted mode.

## E2E Verification

Freestanding verification in QEMU should confirm:

1. boot still succeeds
2. `welcome to Madem-OS!` still appears
3. `kernel_entry=0xC0100000` still appears
4. `user mode ready` still appears
5. keyboard input still reaches the ring-3 user app

These checks prove:

- the higher-half kernel still works
- the copied user app still runs
- syscalls still bridge the isolated user app to the terminal stack

## Definition Of Done

This stage is complete when:

- the current user app no longer runs from the kernel image mapping
- the process owns a page-directory pointer and VM descriptor
- the user app runs from a dedicated low user mapping
- the higher-half kernel mapping can be marked supervisor-only
- new VM policy code has hosted unit-test coverage
- existing unit tests pass
- existing e2e tests pass

At that point, the system will finally have the right base for the next real
milestone:

- adding user-pointer validation
- then adding process creation / running one app from another
