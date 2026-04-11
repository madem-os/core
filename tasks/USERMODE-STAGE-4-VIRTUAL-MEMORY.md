# Usermode Stage 4: Paging, Higher-Half Kernel, And Isolation Foundation

## Scope

This document defines the next large stage after the first working ring-3
vertical slice.

The current system already has:

- one ring-3 user process
- `int 0x80` syscalls
- `read` / `write`
- TTY-backed stdin/stdout/stderr
- a shared linear address space between kernel and user code

The priority for the next stage is:

1. turn on paging safely
2. migrate the kernel to a higher-half virtual address layout
3. preserve all current behavior with unit and e2e coverage
4. only then build per-process isolation on top of that cleaner base

This stage is therefore not only about isolation. It is about first moving the
kernel to a cleaner virtual-memory architecture and then using that foundation
for user address spaces.

## Why This Order

For this project, the priority is cleaner architecture before additional
process features.

The higher-half kernel is not required for isolation in the abstract, but it
does give a better long-term structure for:

- separating kernel and user virtual regions
- reducing accidental overlap with user mappings
- making future loader and process work easier to reason about
- making memory-management bugs less likely to silently corrupt the kernel

So the intended order is:

- paging on
- higher-half kernel
- isolated user address spaces
- later `spawn/exec`

That is the right order for this codebase because architectural clarity is a
priority in its own right.

## Goal

After this full stage, MademOS should be able to:

1. enable paging
2. execute the kernel in a higher-half virtual region
3. preserve the current usermode and terminal behavior
4. introduce one per-process page directory model on top of the higher-half
   kernel mapping
5. run the current built-in user process in a dedicated user virtual region
6. validate user pointers in syscalls

The stage should be implemented and committed in smaller sub-steps, each of
which should keep the current tests green.

## Non-Goals

This stage does not yet include:

- multiple concurrent processes
- scheduling
- `spawn`, `exec`, `fork`, or `wait`
- loading executables from disk
- copy-on-write
- demand paging
- a general heap allocator for user processes

Those come later.

## Stage Breakdown

### Stage 4.1: Enable Paging With Identity Mapping Only

Goal:

- turn paging on without changing any virtual addresses yet

Behavior:

- kernel still executes at the same low linear addresses as today
- page tables identity-map the memory currently required by the kernel and boot
  environment
- usermode, syscalls, TTY, console, and e2e behavior remain unchanged

Why this comes first:

- it isolates the most delicate transition
- it lets the project verify that paging itself is correct before any higher
  half or isolation work

Definition of done for 4.1:

- paging is enabled
- the kernel still boots
- the current user process still runs
- `./tests/run-unit.sh` passes
- `python3 tests/e2e.py` passes

### Stage 4.2: Migrate The Kernel To The Higher Half

Goal:

- keep the kernel physically loaded low
- execute it at a high virtual address

Recommended model:

- kernel physical load remains at `0x00100000`
- linker virtual base moves to a higher-half address, for example
  `0xC0100000`
- early page tables map both:
  - a temporary low identity window
  - the high-half kernel window
- boot/entry code switches paging on while both aliases exist
- control then transfers into the high-half kernel entry

Why this is separate from 4.1:

- linker and virtual-address changes are a different class of bug than “paging
  turns on”
- debugging is much easier if those changes are not mixed together

Definition of done for 4.2:

- the kernel runs from high virtual addresses
- the current ring-3 user program still works
- the current terminal/input/output path still works
- all current tests still pass

### Stage 4.3: Introduce User Virtual Window And Per-Process CR3

Goal:

- add one isolated user virtual region on top of the higher-half kernel

This is the first point where the OS gets real memory separation, not only a
cleaner kernel layout.

The architectural rule should be explicit:

- when a process is active, the kernel also runs with that process's CR3
  loaded

That means:

- syscalls execute with shared kernel mappings plus the current process's user
  mappings visible
- future context switches will switch CR3 as part of changing the current
  process

Definition of done for 4.3:

- `struct process` owns a page-directory pointer
- one user virtual region is mapped for the current process
- the built-in user program runs from that user region, not the kernel-linked
  address
- the current usermode app still works through syscalls

### Stage 4.4: Validate User Pointers In Syscalls

Goal:

- stop trusting raw user pointers passed to `read` / `write`

Validation should be:

1. user-range check
2. minimal page-table walk

This should reject:

- kernel pointers passed from user space
- ranges that cross past the user limit
- unmapped or non-user pages
- non-writable pages when the syscall needs write access

Definition of done for 4.4:

- `read` and `write` reject invalid user buffers
- negative e2e cases exist
- current positive e2e cases still pass

## Higher-Half Kernel Design

### Physical And Virtual Model

The recommended first higher-half design is:

- physical kernel load address stays low:
  - `KERNEL_PHYS_BASE = 0x00100000`
- virtual kernel base moves high:
  - for example `KERNEL_VIRT_BASE = 0xC0000000`
- linked kernel entry becomes:
  - `KERNEL_ENTRY_VIRT = KERNEL_VIRT_BASE + 0x00100000`

The exact constant can vary, but the design should commit to a true high-half
mapping rather than a vague “higher than before” rule.

### Transition Model

During the migration step, the early page tables should provide:

- low identity mapping for the currently executing boot/kernel code
- high-half mapping for the linked kernel image

Then the system should:

1. enable paging
2. continue briefly under the low alias
3. jump to the high-half kernel address
4. continue normal kernel execution from there

The low alias may be kept temporarily after bring-up for simplicity. Removing
it can be a later cleanup.

## User Virtual Window Contract

Once the kernel is high-half, the user window should become a hard contract in
the lower part of the address space.

Suggested first constants:

- `USER_BASE      = 0x00400000`
- `USER_TEXT_BASE = 0x00400000`
- `USER_HEAP_BASE = 0x00800000` (reserved placeholder)
- `USER_STACK_TOP = 0x00C00000`
- `USER_LIMIT     = 0x00C00000`

Important rule:

- the user window is a fixed contract
- the kernel must prove it does not use that range

That proof should cover:

- kernel static sections
- bootstrap stack
- bootstrap page tables/directories
- frame allocator metadata
- VGA/device windows already in use
- built-in user image blob in the kernel image

## Paging Model

Use standard 32-bit non-PAE 4 KiB paging.

### Kernel Mapping Policy

Kernel pages should be:

- present
- supervisor-only
- writable where needed
- identically mapped in the kernel region of every process address space

### User Mapping Policy

User pages should be:

- present
- user-accessible
- writable for stack/data pages

The first isolated user text may still be writable if that keeps the bring-up
simple. Read-only text can be added later.

## Process Address-Space Shape

The process should grow a small VM descriptor:

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

This is enough for the current stage. It avoids scattering user-range
constants across unrelated modules.

## Physical Frame Allocation

The first version should use a tiny page-sized bump allocator for physical
frames.

The allocator must never hand out pages occupied by:

- kernel image sections
- bootstrap stack
- bootstrap page directory/table structures
- built-in user image blob stored in the kernel image
- VGA/device memory windows already in use
- allocator metadata itself

The first implementation should explicitly define:

- allocator start frame
- provisional allocator limit
- reserved exclusions

## User Stack

The first user stack should reserve a guard gap now.

Recommended first model:

- map a small user stack region
- leave one unmapped page below it as a guard page

Even before full page-fault handling, this is the right contract to establish.

## Syscall Pointer Validation

Validation should be explicit and honest.

Suggested API:

```c
int vm_validate_user_buffer(const void *ptr, uint32_t len, int writeable);
```

Validation should do:

1. range check
2. wraparound check
3. page-table walk across every touched page

It should verify:

- buffer stays within the process user range
- every touched page is present
- every touched page is marked user-accessible
- every touched page is writable when required

## Proposed Files

New files likely needed in this overall stage:

- `include/kernel/vm.h`
- `src/kernel/vm.c`
- `include/arch/x86/paging.h`
- `src/arch/x86/paging.c`

Likely files to change:

- `link.ld`
- `bootS.asm`
- `include/kernel/process.h`
- `src/kernel/process.c`
- `kernel.c`
- `src/arch/x86/usermode_stub.S`
- `src/arch/x86/syscall.c` or generic syscall layer for validation integration

## Task Breakdown

The recommended implementation and commit order is:

1. **Task 4.1**
   Paging-on only with identity mapping.

2. **Task 4.2**
   Higher-half kernel migration with temporary low alias.

3. **Task 4.3**
   Process VM descriptor and per-process CR3 groundwork.

4. **Task 4.4**
   User virtual window mapping plus copying the built-in user image into that
   region.

5. **Task 4.5**
   Syscall pointer validation plus negative e2e cases.

Each task should preserve current behavior and keep the current tests green
before moving on.

## Test Plan

### Hosted Tests

Hosted unit tests should cover:

- range and wraparound checks
- page-walk validation over synthetic PDE/PTE state
- process VM structure initialization

### Freestanding / E2E Verification

After each task, verify:

- the kernel still boots
- `user mode ready` still appears
- keyboard input still reaches the user process

Later negative e2e cases for validation should include:

- `write(1, bad_kernel_pointer, len)` returns `-1`
- `read(0, bad_kernel_pointer, len)` returns `-1`
- a buffer crossing past `USER_LIMIT` returns `-1`

## Definition Of Done

This whole stage is complete when:

- paging is enabled
- the kernel runs in the higher half
- the current user app runs from mapped user virtual memory
- each process owns a page-directory pointer
- syscalls validate user buffers before dereferencing them
- current unit tests pass
- current e2e tests pass

At that point, the project will have the right foundation for the following
milestone:

- executing one user app from another
- building `spawn/exec` on top of real address spaces
