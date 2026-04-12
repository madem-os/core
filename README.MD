# MademOS

MademOS is a hobby x86 operating system project built from scratch.

It currently boots through a custom NASM bootloader into a 32-bit freestanding
kernel, enables paging, jumps into a higher-half kernel mapping, handles
keyboard input through IRQs, routes input through an input and TTY stack,
renders terminal output through a text console layered over a generic display
interface with an x86 VGA backend, and runs built-in ring-3 user programs from
isolated low user mappings through a minimal syscall ABI.

Live demo:

- [https://madem-os.github.io/core/](https://madem-os.github.io/core/)

## Current Status

The current stack is:

- boot sector + stage-2 loader
- protected-mode x86 kernel
- bootstrap paging bring-up
- higher-half kernel mapping
- IDT / PIC / IRQ infrastructure
- x86 keyboard driver
- input translation and buffering
- TTY raw/cooked behavior
- text console output layer
- x86 VGA display backend
- page fault panic path
- minimal process + fd model
- process-owned VM state
- kernel `read` / `write` dispatch
- `int 0x80` syscall entry
- ring 3 transition
- built-in user ELF loading
- multiple built-in terminal user programs

The current kernel boots, enables paging, continues execution from the higher
half, initializes the stack above, enters user mode through a process-owned
page directory, and is covered by both hosted unit tests and QEMU end-to-end
tests.

## Architecture Diagram

```text
         boot sector + stage 2 loader
                      |
                      v
          +-----------+------------+
          | Bootstrap Paging +     |
          | Higher-Half Jump       |
          | bootS.asm / link.ld    |
          +-----------+------------+
                      |
                      v
          +-----------+------------+
          | Higher-Half Kernel     |
          | kernel_entry @ 0xC...  |
          +-----------+------------+
                      |
          +-----------+------------+
          | Built-in User Program  |
          | Registry + Embedded    |
          | User ELF Images        |
          | src/kernel/user_programs|
          +-----------+------------+
                      |
                ELF parse + map
                      |
          +-----------v------------+
          | VM Layer / Page Tables |
          | src/kernel/vm          |
          +-----------+------------+
                      |
          isolated low user mapping
                      |
                      v
                      +-----------------------+
                      |   Built-in User App   |
                      | user/apps/echo_line   |
                      +-----------+-----------+
                                  |
                     read/write via int 0x80
                                  |
                      +-----------v-----------+
                      |   x86 Syscall Entry   |
                      | src/arch/x86/syscall  |
                      +-----------+-----------+
                                  |
                      +-----------v-----------+
                      | Kernel Syscall Layer  |
                      | src/kernel/syscall    |
                      +-----------+-----------+
                                  |
              +-------------------+-------------------+
              |                                       |
      +-------v--------+                      +-------v--------+
      |  Kernel I/O    |                      | Process / FDs  |
      | src/kernel/io  |<-------------------->|src/kernel/proc |
      +-------+--------+                      +----------------+
              |
      fd 0/1/2 -> active TTY
              |
      +-------v--------+
      |      TTY       |
      |   src/tty      |
      +---+--------+---+
          |        |
   input/read      | output/write
          |        |
   +------v--+   +-v------------------+
   | Input   |   |   Text Console     |
   | Layer   |   | src/console/text   |
   |src/input|   +---------+----------+
   +----^----+             |
        |                  v
        |       +----------+-----------+
        |       | Display Abstraction  |
        |       | src/console/display  |
        |       +----------+-----------+
        |                  |
        |                  v
        |       +----------+-----------+
        |       | x86 VGA Display      |
        |       | arch/x86/drivers/vga |
        |       +----------+-----------+
        |                  |
        |                  v
        |         VGA text buffer + cursor


   Keyboard path:

   PS/2 keyboard IRQ1
        |
        v
   arch/x86/drivers/keyboard
        |
        v
   input_handle_scancode(...)
```

## Milestones

### 2026-04-12: Paging, Higher-Half Kernel, and Isolated User Mappings

This milestone turns the first usermode vertical slice into a real paged
execution model:

- bootstrap paging is enabled with an initial identity mapping
- the kernel is linked high and entered through a higher-half virtual address
- the first process now owns its own page directory and isolated low user text
  and stack mappings
- kernel mappings in that process address space are supervisor-only
- page faults now fail through an explicit panic path instead of silent breakage
- built-in user programs are built as separate ELF images, embedded into the
  kernel, and selected through a small registry
- the build/runtime flow now supports multiple built-in user apps

This is the first milestone where the kernel virtual-memory layout looks like a
real OS kernel instead of a temporary shared-address-space prototype.

Relevant design docs:

- [tasks/USERMODE-STAGE-4-VIRTUAL-MEMORY.md](tasks/USERMODE-STAGE-4-VIRTUAL-MEMORY.md)
- [tasks/USERMODE-STAGE-4.2-HIGHER-HALF-KERNEL.md](tasks/USERMODE-STAGE-4.2-HIGHER-HALF-KERNEL.md)
- [tasks/USERMODE-STAGE-4.3-ISOLATED-USER-MAPPING.md](tasks/USERMODE-STAGE-4.3-ISOLATED-USER-MAPPING.md)

### 2026-04-11: Minimal Usermode Vertical Slice

Today’s milestone completes the first end-to-end usermode path:

- one built-in ring-3 process
- `int 0x80` syscalls for `read` and `write`
- stdin/stdout/stderr wired through the kernel fd/process layer
- user output rendered through `tty -> text_console -> vga_display`
- user input delivered through `keyboard -> input -> tty -> read`

The current built-in user program writes a startup message and then acts as a
simple terminal echo loop.

Relevant design docs:

- [tasks/USERMODE-MINIMAL-PLAN.md](tasks/USERMODE-MINIMAL-PLAN.md)
- [tasks/USERMODE-STAGE-1-KERNEL-IO.md](tasks/USERMODE-STAGE-1-KERNEL-IO.md)
- [tasks/USERMODE-STAGE-2-SYSCALLS.md](tasks/USERMODE-STAGE-2-SYSCALLS.md)
- [tasks/USERMODE-STAGE-3-RING3-PROCESS.md](tasks/USERMODE-STAGE-3-RING3-PROCESS.md)

### Earlier Milestone: TTY + Text Console Stack

This milestone introduced the first correct terminal architecture for the
project:

- keyboard IRQs feed the input system
- the input system translates scancodes into terminal bytes
- the TTY owns raw/cooked behavior and echo policy
- the text console owns terminal-visible rendering behavior
- the VGA backend is isolated under the display abstraction

Relevant design docs:

- [tasks/INPUT-SYSTEM-DESIGN.md](tasks/INPUT-SYSTEM-DESIGN.md)
- [tasks/TTY-READ-DESIGN.md](tasks/TTY-READ-DESIGN.md)
- [tasks/TTY-MODULE-TDD.md](tasks/TTY-MODULE-TDD.md)
- [tasks/CONSOLE-DRIVER-DESIGN.md](tasks/CONSOLE-DRIVER-DESIGN.md)

### Earlier Milestone: First User Input Path

Before the current TTY/text-console design, the project had an earlier,
simpler input path based on the old `console.c` path. That milestone proved the
keyboard interrupt path and basic user interaction, but it mixed input,
terminal behavior, and VGA rendering too tightly.

That phase was useful as a proof of concept, but it has now been replaced by
the input -> TTY -> text_console -> vga_display layering.

Relevant design docs:

- [tasks/KEYBOARD-IRQ-DESIGN.md](tasks/KEYBOARD-IRQ-DESIGN.md)
- [tasks/KEYBOARD-STDIO-ROADMAP.md](tasks/KEYBOARD-STDIO-ROADMAP.md)

### First Milestone: Bootloader + Kernel Bring-Up

The first milestone was getting the machine to boot reliably into the kernel:

- BIOS boot sector
- stage-2 loader
- protected-mode transition
- kernel loading from disk
- first VGA output

This milestone also included fixing the broken ATA PIO sector loading path that
was exposed once the kernel stopped fitting in the earliest loaded sector.

Relevant notes:

- [tasks/FIX-BOOT-READ-SECOTRS-NOTES.md](tasks/FIX-BOOT-READ-SECOTRS-NOTES.md)

## Repository Layout

- `bootS.asm`: boot sector + stage-2 loader
- `kernel.c`: kernel entry point and top-level subsystem wiring
- `link.ld`: linker script for the kernel image
- `src/arch/x86/`: x86 architecture support code
- `src/arch/x86/drivers/`: x86-specific hardware drivers
- `src/kernel/`: kernel process, I/O, and syscall dispatch layers
- `src/input/`: terminal input translation and buffering
- `src/tty/`: TTY behavior and terminal I/O semantics
- `src/console/`: generic display and text-console layers
- `user/`: built-in userspace code and syscall wrappers
- `include/`: headers for the kernel code
- `tests/`: hosted unit tests and QEMU e2e tests
- `tasks/`: design docs and engineering notes
- `bin/`: generated build artifacts

## Important Modules

- `src/arch/x86/idt.c`: IDT setup
- `src/arch/x86/irq.c`: IRQ registration and dispatch
- `src/arch/x86/pic.c`: PIC control
- `src/arch/x86/ports.c`: raw x86 port I/O helpers
- `src/arch/x86/drivers/keyboard.c`: PS/2 keyboard IRQ driver
- `src/arch/x86/drivers/vga_display.c`: x86 VGA text-mode display backend
- `src/arch/x86/exceptions.c`: page fault reporting and fatal exception path
- `src/arch/x86/syscall.c`: x86 `int 0x80` entry path
- `src/arch/x86/usermode.c`: x86 ring-3 entry helper
- `src/kernel/bootstrap_paging.c`: testable bootstrap page-table construction
- `src/input/input.c`: scancode-to-byte translation and buffering
- `src/input/input_ring.c`: byte ring buffer for the input layer
- `src/kernel/process.c`: minimal process + stdio descriptor wiring
- `src/kernel/io.c`: kernel `kread` / `kwrite`
- `src/kernel/user_programs.c`: built-in user-program registry
- `src/kernel/vm.c`: user ELF parsing and per-process VM layout
- `src/kernel/syscall.c`: generic syscall dispatcher
- `src/tty/tty.c`: raw/cooked TTY behavior and TTY writes
- `src/console/text_console.c`: terminal-visible output behavior
- `src/console/display.c`: generic display object initialization
- `user/lib/syscall.c`: userspace syscall wrappers
- `user/apps/echo_line.c`: first built-in user program

## Build Output

Running `build.sh` creates all generated files under `bin/`:

- `bin/bootS.bin`
- `bin/obj/...`
- `bin/kernel.elf`
- `bin/kernel.bin`
- `bin/disk.img`
- `bin/user/*.elf`

It also refreshes the browser-demo disk image at:

- `docs/assets/mademos.img`

## Browser Demo

The repository now includes a static browser demo under:

- `docs/index.html`

It boots the current `disk.img` through `v86`, so the demo tracks the normal
OS build instead of a hand-maintained artifact. This is suitable for GitHub
Pages hosting.

Live demo:

- [https://madem-os.github.io/core/](https://madem-os.github.io/core/)

## Requirements

### Common

- `nasm`
- `qemu-system-i386`
- `truncate` or `gtruncate`

### Linux

You need:

- `gcc` or an ELF cross-compiler
- `ld`
- `objcopy`

Typical Debian/Ubuntu packages:

```bash
sudo apt install build-essential binutils nasm qemu-system-x86
```

### macOS

You need:

- `nasm`
- `qemu`
- `lld`
- either LLVM `llvm-objcopy` or GNU `objcopy`
- a compiler that can emit `i386-none-elf` objects

Homebrew example:

```bash
brew install nasm qemu lld llvm binutils
```

`build.sh` knows how to find the Homebrew toolchain on macOS.

## Build And Run

From the project root:

```bash
./build.sh
```

This will:

1. assemble the bootloader
2. compile and link the kernel
3. generate `bin/disk.img`
4. start QEMU with that image

If you only want to build without launching QEMU:

```bash
./build.sh --build-only
```

Manual run example:

```bash
qemu-system-i386 -m 512 -drive file=bin/disk.img,format=raw
```

## Tests

### E2E

There is a single end-to-end runner under `tests/e2e.py`.

It:

1. builds the image once
2. boots QEMU once
3. runs multiple checks sequentially against the same live VM
4. uses the QEMU monitor to inspect VGA memory and inject keyboard input

Run all e2e checks with:

```bash
python3 tests/e2e.py
```

Run one named check with:

```bash
python3 tests/e2e.py boot_text
python3 tests/e2e.py keyboard_input
```

The current e2e flow verifies:

- the boot banner appears
- user mode starts and prints `user mode ready`
- keyboard input reaches the running user process

### Unit Tests

Hosted unit tests live under `tests/unit/` and run via:

```bash
./tests/run-unit.sh
```

They currently cover:

- input translation
- input ring buffer behavior
- bootstrap paging layout
- kernel process and fd dispatch
- generic syscall dispatch
- TTY behavior
- text console behavior
- VM layout and isolated user mapping policy
- built-in user-program registry

## Project Notes

- This is an educational hobby OS project, not a production OS.
- `src/` + `include/` is the current C project layout.
- `bin/` is generated and ignored by git.
- The current usermode path now runs through a dedicated per-process page
  directory, but it is still intentionally small: one active process, one low
  user image, one user stack, and no general frame allocator or demand paging.
