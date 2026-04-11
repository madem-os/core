# Usermode Stage 4: Minimal Virtual Memory And Address-Space Isolation

## Scope

This document defines the next stage after the first working ring-3 vertical
slice.

The current system already has:

- one ring-3 user process
- `int 0x80` syscalls
- `read` / `write`
- TTY-backed stdin/stdout/stderr
- a shared linear address space between kernel and user code

This stage adds the minimum virtual-memory foundation needed for:

- real user/kernel memory isolation
- per-process address spaces
- safe user-pointer handling in syscalls
- a clean base for a later `spawn/exec` milestone

This stage does **not** yet add multi-process execution or an executable
loader. It only builds the memory model those features will need.

## Goal

After this stage, MademOS should be able to:

1. enable paging
2. keep the kernel mapped at stable virtual addresses
3. create one user address space that is separate from the kernel’s writable
   memory
4. run the current built-in user process inside that address space
5. reject invalid user pointers in `read` / `write` syscalls
6. preserve the existing terminal behavior and e2e tests

The success condition is:

- the current user app still runs
- user memory is no longer the same writable region as kernel memory
- syscall pointer arguments are checked against the user virtual range

## Non-Goals

This stage does not include:

- loading executables from disk
- multiple concurrent processes
- a scheduler
- `spawn`, `exec`, `fork`, or `wait`
- copy-on-write
- demand paging
- a heap allocator for user processes
- swapping or page reclaim

Those belong to later stages.

## Why This Stage Comes Next

The next milestone the project wants is:

- one user app launching another user app
- memory isolation

That requires a real process boundary. Right now the OS has privilege
separation, but not memory separation. A user program runs in ring 3, yet its
code and buffers still live in the same linear address space as the kernel.

Before the OS can safely create and load additional user processes, it needs:

- paging
- one per-process page directory
- a stable kernel mapping
- a defined user virtual range
- syscall pointer validation

Without those pieces, `spawn/exec` would only create more shared-memory
processes, which is not the intended direction.

## Design Principles

- Keep the first paging model small and explicit.
- Preserve the current kernel high-half-free layout unless a relocation is
  truly necessary.
- Prefer a fixed, easy-to-debug user virtual layout over a flexible allocator.
- Share the kernel mapping across all processes.
- Delay advanced VM features until there is a real loader and more than one
  process.

## High-Level Design

The first isolated address-space model should be:

- identity-mapped kernel physical memory in the low region used today
- one user virtual window reserved below the kernel-used range
- per-process page directory
- shared kernel PDEs
- process-private user PDE/PTEs

Conceptually:

`user virtual pages -> process page tables -> physical user pages`

and:

`kernel virtual pages -> shared kernel page tables -> kernel physical pages`

The current ring-3 user program can still be built into the image, but before
entering user mode the kernel should copy its bytes into mapped user pages and
jump to the **user virtual entry address**, not the kernel link address.

## Proposed Virtual Address Layout

Keep the first layout intentionally simple.

Suggested user layout:

- `0x00400000` - user text/data base
- `0x00400000` - first user program entry region
- `0x00800000` - optional future heap base placeholder
- `0x00C00000` - user stack top

Suggested kernel rule:

- kernel keeps its current low memory identity mapping
- user mappings must stay below the kernel-used image and stack region

The exact user addresses can be adjusted, but the important constraints are:

- one fixed user code base
- one fixed user stack top
- no overlap with kernel runtime data

## Paging Model

Use standard 32-bit non-PAE 4 KiB paging.

### First Paging Features

- one page directory for the kernel bootstrap environment
- one page directory for the first user process
- 4 KiB pages
- present / writable / user flags as needed
- shared kernel mappings copied into each process page directory

### Kernel Mapping Policy

Kernel pages should be:

- present
- writable where needed
- supervisor-only

User pages should be:

- present
- writable for stack/data pages
- user-accessible

The initial user text can be writable too if that simplifies the first stage.
Read-only text protection can be added later.

## Memory Objects Needed

The first VM stage should introduce:

### Page Directory / Page Table Types

For example:

```c
typedef uint32_t page_entry_t;

struct page_directory {
    page_entry_t entries[1024];
} __attribute__((aligned(4096)));

struct page_table {
    page_entry_t entries[1024];
} __attribute__((aligned(4096)));
```

### Process Address-Space State

`struct process` should grow to include:

```c
struct process {
    struct file_descriptor fds[3];
    uintptr_t entry_point;
    uintptr_t user_stack_top;
    struct page_directory *page_directory;
};
```

Later stages may add:

- pid
- parent
- state
- saved register context

But they are not required yet.

## Proposed Files

New files:

- `include/kernel/vm.h`
- `src/kernel/vm.c`
- `include/arch/x86/paging.h`
- `src/arch/x86/paging.c`

Likely files to change:

- `include/kernel/process.h`
- `src/kernel/process.c`
- `src/arch/x86/usermode_stub.S` only if CR3 interaction or stack rules need
  small adjustments
- `src/arch/x86/syscall.c` or generic syscall layer if validation helpers are
  integrated there
- `kernel.c`
- `build.sh` only if a user-image copy step is introduced

## Module Responsibilities

### `arch/x86/paging`

Responsibilities:

- own x86 paging instruction helpers
- load CR3
- enable paging in CR0
- expose page-table flag constants
- provide small helpers for page-directory/page-table entry construction

This module should stay x86-specific and instruction-focused.

### `kernel/vm`

Responsibilities:

- create the bootstrap kernel page directory
- create one process page directory
- map user pages
- copy the built-in user image into user memory
- validate user pointers against the active process address space contract

This module owns policy, not the raw paging instructions.

## Bootstrap Strategy

The easiest first implementation is:

1. build a page directory that identity-maps the memory currently used by the
   kernel and boot environment
2. enable paging
3. build a second page directory for the first user process
4. copy shared kernel mappings into it
5. allocate and map:
   - one user text/data region
   - one user stack page or a few stack pages
6. copy the built-in user program bytes into the mapped user text region
7. set:
   - `process.entry_point = USER_TEXT_BASE + user_program_offset`
   - `process.user_stack_top = USER_STACK_TOP`
8. switch to that process page directory before entering ring 3

This is enough to isolate the first process without introducing a full loader.

## User Program Placement

There are two reasonable ways to handle the existing built-in user app.

### Option A: Keep It Linked Into The Kernel, Then Copy It

The kernel linker script exports symbols for the user code blob:

```c
extern uint8_t _user_image_start[];
extern uint8_t _user_image_end[];
extern uintptr_t _user_image_entry;
```

Then VM setup:

- allocates user pages
- copies the user image bytes into `USER_TEXT_BASE`
- computes the user entry virtual address

This is the recommended first step because it avoids building a file loader
and still forces the kernel to execute the user program from isolated pages.

### Option B: Keep Executing The Kernel-Linked Address

Do not do this. It preserves the current shared-memory model and defeats the
purpose of this stage.

## Pointer Validation Rules

This stage should add explicit user-pointer validation helpers.

Suggested API:

```c
int vm_validate_user_buffer(const void *ptr, uint32_t len, int writeable);
```

For the first version, validation can be range-based:

- pointer must be fully inside the defined user virtual range
- range must not wrap around
- zero-length buffers are allowed

Later this can evolve into real page-presence checks, but range validation is
already a meaningful improvement over the current unchecked behavior.

## Syscall Changes Required

The current `read` / `write` syscalls trust user pointers. That must change.

Required updates:

- `SYS_READ`:
  - validate destination buffer as writable user memory
- `SYS_WRITE`:
  - validate source buffer as readable user memory

If validation fails:

- return `-1` for now

That is enough for the current syscall style.

## Process Integration

The current single-process model stays in place, but it becomes address-space
aware.

The first kernel process initialization sequence should become:

1. initialize terminal/input/output stack
2. initialize paging
3. initialize one `struct process`
4. wire TTY stdio
5. create the process page directory
6. map user text and stack
7. copy the built-in user image into user space
8. set process entry and stack pointers to user virtual addresses
9. set current process
10. switch CR3 to the process page directory
11. enter ring 3

## Hosted Test Plan

Hosted unit tests cannot execute paging instructions, so the test split should
be:

### Hosted Tests

Test generic VM policy in `kernel/vm`:

- user-range validation success/failure
- overflow handling in pointer-range checks
- process address-space field initialization

These tests should not try to execute `mov %cr3` or `mov %cr0`.

### Freestanding / E2E Verification

Verify in QEMU that:

- the kernel still boots
- the first user process still prints `user mode ready`
- keyboard input still reaches the user process
- invalid user pointers can be tested later through a small fault-injection
  user app

## Recommended Implementation Order

1. introduce x86 paging instruction helpers
2. introduce generic VM policy module
3. build and enable a bootstrap identity-mapped page directory
4. extend `struct process` with `page_directory`
5. build one per-process page directory with copied kernel mappings
6. map user text and user stack
7. copy the current built-in user image into user pages
8. switch the current process to that page directory
9. add syscall pointer validation
10. update e2e tests if needed

## Open Design Choices

These should be resolved before implementation starts:

1. **How much kernel memory to identity-map at first?**
   Recommendation: map enough low memory to cover the current kernel image,
   stack, page structures, VGA memory, and known device regions already in use.

2. **How are physical pages allocated for the first version?**
   Recommendation: use a tiny static bump allocator for page-sized frames.
   A full physical memory manager is not needed yet.

3. **How is the user image copied and bounded?**
   Recommendation: add linker symbols for a dedicated user image section.

4. **Should user text be writable in the first version?**
   Recommendation: yes, if it keeps the first stage simpler. Tighten later.

## Definition Of Done

This stage is complete when:

- paging is enabled
- the current ring-3 app runs from mapped user virtual memory
- the kernel and user memory are no longer the same writable region
- each process owns a page directory pointer
- syscalls validate user buffers before dereferencing them
- the current unit tests pass
- the current e2e tests pass

At that point, the OS will have the minimum memory-isolation base needed for
the following milestone:

- loading and executing a second user app from the first one
- adding real process creation and replacement APIs on top
