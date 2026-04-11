# MademOS

MademOS is a hobby x86 operating system project built from scratch.

It currently boots through a custom NASM bootloader into a 32-bit freestanding
kernel, handles keyboard input through IRQs, routes input through an input and
TTY stack, and renders terminal output through a text console layered over a
generic display interface with an x86 VGA backend.

## Current Status

The current stack is:

- boot sector + stage-2 loader
- protected-mode x86 kernel
- IDT / PIC / IRQ infrastructure
- x86 keyboard driver
- input translation and buffering
- TTY raw/cooked behavior
- text console output layer
- x86 VGA display backend

The current kernel boots, initializes the stack above, writes through the TTY
path, and is covered by both hosted unit tests and QEMU end-to-end tests.

## Milestones

### 2026-04-11: TTY + Text Console Stack

Today’s milestone is the first correct terminal architecture for the project:

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
- [tasks/USERMODE-MINIMAL-PLAN.md](tasks/USERMODE-MINIMAL-PLAN.md)

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
- `src/input/`: terminal input translation and buffering
- `src/tty/`: TTY behavior and terminal I/O semantics
- `src/console/`: generic display and text-console layers
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
- `src/input/input.c`: scancode-to-byte translation and buffering
- `src/input/input_ring.c`: byte ring buffer for the input layer
- `src/tty/tty.c`: raw/cooked TTY behavior and TTY writes
- `src/console/text_console.c`: terminal-visible output behavior
- `src/console/display.c`: generic display object initialization

## Build Output

Running `build.sh` creates all generated files under `bin/`:

- `bin/bootS.bin`
- `bin/obj/...`
- `bin/kernel.elf`
- `bin/kernel.bin`
- `bin/disk.img`

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

### Unit Tests

Hosted unit tests live under `tests/unit/` and run via:

```bash
./tests/run-unit.sh
```

They currently cover:

- input translation
- input ring buffer behavior
- TTY behavior
- text console behavior

## Project Notes

- This is an educational hobby OS project, not a production OS.
- `src/` + `include/` is the current C project layout.
- `bin/` is generated and ignored by git.
- The next major direction is minimal user mode with syscalls and one terminal
  process, described in [tasks/USERMODE-MINIMAL-PLAN.md](tasks/USERMODE-MINIMAL-PLAN.md).
