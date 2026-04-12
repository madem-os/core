# Higher-Half Kernel Proof: Low Alias Removal Notes

## Summary

This note documents exactly what was changed in this session, why those changes
were necessary, what broke when the temporary low kernel mapping was removed,
and how the final system was verified.

The goal of the session was:

- stop relying on the temporary low identity alias after the kernel has already
  transitioned into the higher half
- prove that the kernel, interrupts, syscalls, console output, and usermode
  execution all work without that alias

This was not just a one-line paging change. Removing the low alias exposed
several real hidden dependencies that had been masked by the old mapping.

## Why This Work Was Needed

After the earlier higher-half migration, the system still kept the low alias
for the kernel image and low kernel memory in place.

That transitional setup is useful during bring-up, but it leaves an important
question unanswered:

- is the kernel truly higher-half clean?
- or does it still accidentally depend on low virtual addresses?

As long as both aliases exist, a bug can remain invisible. Code may still use a
low address by mistake and appear to work only because the old identity mapping
is still present.

So the purpose of this session was to convert the higher-half migration from
"works with a transitional crutch" into "proven to run without the crutch."

## Initial State Before The Session

Before these changes:

- the boot path installed a bootstrap page directory at physical `0x00020000`
- that page directory mapped both:
  - the low identity window
  - the higher-half kernel alias
- the kernel did enter `kernel_entry` at a higher-half address
- process page directories also carried a low kernel PDE at directory entry 0
- the kernel stack and some privileged CPU structures were still effectively
  tied to the old low-address assumptions

That meant the system was "higher-half booted" but not yet "higher-half proven."

## Main Design Decision

The decision taken in this session was:

1. keep the existing bootstrap paging layout long enough to reach the higher-half kernel
2. once the kernel is executing in the higher half and has installed its own
   runtime descriptor state, explicitly remove the bootstrap low alias
3. stop carrying a low kernel mapping into process page directories
4. fix every dependency that turns out to have been secretly relying on the
   old mapping

This approach gives a much stronger guarantee than leaving PDE 0 around
indefinitely.

## What Was Changed

### 1. Added Runtime Higher-Half GDT/TSS Installation

New files:

- `include/arch/x86/gdt.h`
- `src/arch/x86/gdt.c`

Why:

- the bootloader-created GDT and TSS live in low memory
- once the low alias is removed, the CPU must not keep depending on descriptor
  tables that are only reachable through the low mapping
- the kernel needs a runtime GDT/TSS installation that is itself reachable in
  the higher half

What this code does:

- defines a kernel-owned runtime GDT
- recreates the kernel code/data and user code/data segments
- defines a runtime TSS
- sets `esp0` in the TSS to the higher-half kernel stack top
- loads the runtime GDT
- loads the task register with the new TSS selector

This change is what makes it safe to stop depending on the bootloader’s low
descriptor state after entering the kernel proper.

### 2. Added Low-Level Helpers For `lgdt` And `ltr`

Changed files:

- `include/arch/x86/lowlevel.h`
- `src/arch/x86/lowlevel.S`

Why:

- the new runtime GDT/TSS setup needed minimal architecture wrappers for
  loading the GDT and task register
- this follows the project’s existing pattern of keeping privileged assembly
  helpers tiny and policy-free

Added helpers:

- `x86_load_gdt(const void *gdt_ptr)`
- `x86_load_task_register(uint16_t selector)`

### 3. Moved The First Kernel Stack To A Higher-Half Address

Changed file:

- `src/arch/x86/entry.S`

Why:

- the entry stub was still setting `esp` to low virtual address `0x00090000`
- as long as PDE 0 existed, that worked
- after removing the low alias, that stack would no longer be mapped

Fix:

- changed the initial kernel stack to the higher-half alias `0xC0090000`

This removed one direct and immediate dependency on the low mapping.

### 4. Added Explicit Bootstrap Low-Alias Removal

Changed files:

- `include/arch/x86/paging.h`
- `src/arch/x86/paging.c`
- `kernel.c`

Why:

- the whole point of the session was to stop leaving the transitional bootstrap
  low alias in place
- there needed to be one explicit point where the kernel says: from here on,
  higher-half execution must stand on its own

What was added:

- `paging_unmap_bootstrap_low_alias()`

What it does:

- treats the bootstrap page directory at physical `0x00020000` as reachable
  through its higher-half alias
- clears page-directory entry 0
- reloads CR3 so the CPU uses the updated page directory contents

Where it is called:

- in `kmain()`, immediately after `gdt_init()`

Why that order matters:

- first install a runtime higher-half GDT/TSS
- then remove the low bootstrap alias
- only after that continue with the rest of kernel initialization

That sequencing prevents later ring transitions from depending on the
bootloader’s low-memory TSS/GDT.

### 5. Removed The Low Kernel PDE From Process Page Directories

Changed files:

- `include/kernel/vm.h`
- `src/kernel/vm.c`
- `tests/unit/test_vm.c`

Why:

- even if the bootstrap page directory drops PDE 0, process page directories
  were still rebuilding a low kernel mapping
- that would keep the old dependency alive after `vm_activate_process()`
- the kernel should run in the process page directory using only the shared
  higher-half kernel mapping plus user mappings

What changed:

- removed `kernel_low_page_table` from `struct vm_runtime`
- removed low-kernel page-table construction from `vm_build_process_page_tables()`
- stopped populating `page_directory[0]` for the kernel path
- updated the VM unit tests to assert that `page_directory[0] == 0`

This is the key runtime proof step for the post-`CR3` usermode path.

### 6. Fixed Hidden VGA Dependency On The Old Low Mapping

Changed files:

- `src/arch/x86/drivers/vga_display.c`
- `src/kernel/panic.c`

Why:

Once PDE 0 was removed, the first e2e boot failed before the normal boot banner
appeared. Debugging showed that the failure was not in the paging change
itself, but in code that was still using the physical VGA address as if it were
a valid kernel virtual address.

The hidden bug:

- `vga_display.c` still used `0x000B8000`
- `panic.c` fallback output still used `0x000B8000`

Why it previously worked:

- PDE 0 identity-mapped low memory, so dereferencing `0x000B8000` happened to
  succeed

Why it failed after alias removal:

- `0x000B8000` was no longer mapped in the kernel address space
- console writes faulted immediately

Fix:

- switched both paths to the higher-half alias `0xC00B8000`

This was the most important practical proof from the session: the low alias was
indeed hiding a real higher-half bug.

## Regression Investigation During The Session

### First Runtime Failure

After the first implementation pass:

- unit tests passed
- e2e boot failed
- VGA text remained blank

QEMU logging showed repeated page faults while trying to write to `CR2=0x000B8000`.

That established:

- the kernel had successfully progressed far enough to attempt console output
- the crash was due to a stale low VGA virtual address assumption

### Why This Failure Was Valuable

This failure was not accidental noise. It was exactly the kind of bug this work
was meant to expose.

If PDE 0 had been left in place:

- the VGA path would still appear correct
- the system would keep booting
- the kernel would remain only partially migrated to the higher half

The failure therefore confirmed that the session objective was correct and that
the low mapping was still hiding real issues.

## File-By-File Change List

### New Files

- `include/arch/x86/gdt.h`
- `src/arch/x86/gdt.c`

### Modified Files

- `include/arch/x86/lowlevel.h`
- `include/arch/x86/paging.h`
- `include/kernel/vm.h`
- `kernel.c`
- `src/arch/x86/drivers/vga_display.c`
- `src/arch/x86/entry.S`
- `src/arch/x86/lowlevel.S`
- `src/arch/x86/paging.c`
- `src/kernel/panic.c`
- `src/kernel/vm.c`
- `tests/unit/test_vm.c`

### Generated Artifact Updated During Verification

- `docs/assets/mademos.img`

That image changed because the verified build refreshed the browser/demo disk
image as part of the normal build flow.

## Final Runtime Flow After The Changes

The relevant boot/runtime flow is now:

1. bootloader loads the kernel physically low
2. bootloader enables paging with both low and higher-half aliases temporarily present
3. bootloader jumps to higher-half `kernel_entry`
4. `kernel_entry` installs a higher-half kernel stack
5. `kmain()` installs a runtime higher-half GDT/TSS
6. `kmain()` explicitly removes bootstrap PDE 0 and reloads CR3
7. remaining kernel init runs entirely from the higher-half kernel mapping
8. VM code builds process page directories without a low kernel PDE
9. usermode runs with:
   - higher-half supervisor-only kernel mappings
   - low user text mapping
   - low user stack mapping

That is the first session where the higher-half kernel path is actually proven
instead of merely intended.

## What Was Verified

### Unit Tests

Command run:

```bash
./tests/run-unit.sh
```

Result:

- all unit tests passed

This included the updated VM tests that now assert there is no kernel low PDE
in the process page directory path.

### End-To-End Tests

Command run:

```bash
python3 tests/e2e.py
```

Result:

- `boot_text` passed
- `keyboard_input` passed
- `user_page_fault` passed

This means the following all survived low-alias removal:

- kernel boot banner rendering
- higher-half `kernel_entry` print
- keyboard input into the running user process
- usermode syscall path
- page-fault panic behavior

## Important Constraints That Still Remain

This session proved one thing specifically:

- the kernel no longer depends on the temporary low alias to boot and run the
  current single-process usermode flow

It did **not** add:

- a general frame allocator
- demand paging
- multi-process scheduling
- copy-on-write
- syscall pointer validation
- a general higher-half device mapping framework beyond the concrete VGA fix

So this is a correctness proof for the current architecture, not a complete VM
subsystem.

## Practical Lessons From The Session

### 1. Removing Transitional Mappings Is A Real Test

It is not just a cosmetic cleanup. It is an effective way to detect whether the
kernel has genuinely migrated to its intended address model.

### 2. Higher-Half Migration Includes More Than The Linker

Linking the kernel high and jumping high is not sufficient by itself.

The following also must be correct:

- kernel stack addresses
- TSS `esp0`
- GDT/TSS residence and reload behavior
- direct hardware-memory aliases like VGA
- page-directory contents after process activation

### 3. Hidden Dependencies Show Up In Peripheral Paths First

The first visible failure in this session was not syscall entry, not CR3
switching, and not ring 3 transition.

It was the VGA console.

That is typical for this kind of migration: seemingly small low-level device
paths often preserve stale assumptions the longest.

## Final Conclusion

This session completed the missing proof step for the higher-half kernel work.

The kernel no longer merely:

- enables paging
- maps itself high
- enters the higher half

It now also:

- removes the temporary low bootstrap alias
- runs kernel-mode control paths without a low kernel PDE
- survives usermode, syscalls, keyboard input, and page-fault panic handling in
  that stricter configuration

The most important concrete outcome is that a real hidden bug was found and
fixed:

- the VGA console and panic fallback were still depending on low virtual
  `0xB8000`

After fixing that dependency and the runtime descriptor/stack assumptions, the
system passed all unit and e2e tests with the low kernel alias removed.
