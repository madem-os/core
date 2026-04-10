# MademOS

MademOS is a hobby x86 operating system project built from scratch.

The project currently contains:

- a boot sector and stage-2 loader written in NASM
- a tiny 32-bit freestanding kernel written in C
- a linker script for placing the kernel at `0x00100000`
- a `build.sh` script that builds a bootable disk image and runs it in QEMU
- a `src/` + `include/` layout for additional kernel code

At the moment, the kernel boots into protected mode and prints a message to VGA text mode.

## Project Layout

- `bootS.asm`: boot sector + stage-2 loader
- `kernel.c`: main kernel entry translation unit
- `src/mos-ports.c`: low-level x86 port I/O helpers
- `src/mos-stdio.c`: VGA text output helpers
- `src/`: additional C source files compiled recursively into the kernel
- `include/`: shared header files for the kernel code
- `link.ld`: ELF linker script for the kernel
- `build.sh`: build and run script
- `bin/`: generated build artifacts such as `disk.img`, `kernel.elf`, `kernel.bin`, and object files

## Build Output

Running `build.sh` creates all generated files under `bin/`:

- `bin/bootS.bin`
- `bin/obj/kernel.o`
- `bin/obj/src/...`
- `bin/kernel.elf`
- `bin/kernel.bin`
- `bin/disk.img`

## Source Layout

`kernel.c` stays as the main entry point and is always compiled.

In addition, `build.sh` compiles every `*.c` file found under `src/`, including nested subdirectories, and links all of them into the final kernel.

Headers can be placed under `include/` and included like this:

```c
#include "drivers/serial.h"
```

because `build.sh` adds `include/` to the compiler include path.

Current shared modules include:

- `mos-ports.h`: `read_port_byte`, `write_port_byte`, `write_port_word`
- `mos-stdio.h`: `print_char`, `print_line`, `printf`

`move_cursor` currently remains private inside `src/mos-stdio.c`.

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

The current `build.sh` already knows how to find the Homebrew toolchain on macOS.

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

If you only want to build the image without launching QEMU:

```bash
./build.sh --build-only
```

## Build And Run On Linux

```bash
./build.sh
```

If your system tools are on `PATH`, nothing else is needed.

## Build And Run On macOS

```bash
./build.sh
```

If the required Homebrew packages are installed, the script will use them automatically.

## Notes

- This is an educational hobby OS project, not a production OS.
- The disk image is generated during the build and is ignored by git.
- If QEMU GUI output has issues in some macOS environments, the image is still created in `bin/disk.img` and can be run manually.

## C Project Structure Notes

Using a `src/` directory is very common in C projects. It is not mandatory, but it is a standard and widely recognizable layout.

A common setup is:

- `src/` for implementation files
- `include/` for headers
- `tests/` for tests
- `bin/` or `build/` for generated artifacts

For larger C codebases, another common pattern is:

- public headers under `include/`
- private, module-local headers next to the `.c` files inside `src/`

So yes, C projects often use `src/`, and many also use a separate `include/` folder. For this project, `kernel.c` at the root plus recursive `src/` and `include/` is a perfectly reasonable modern structure.

Example manual run:

```bash
qemu-system-i386 -m 512 -drive file=bin/disk.img,format=raw
```

## E2E Test

There is a single end-to-end runner under `tests/e2e.py`.

It does the following:

1. runs `./build.sh --build-only`
2. boots `bin/disk.img` in headless QEMU once
3. runs multiple checks sequentially against the same live QEMU session
4. uses the QEMU monitor to inspect VGA memory and inject keyboard input

Run all e2e checks with:

```bash
python3 tests/e2e.py
```

Run one named check with:

```bash
python3 tests/e2e.py boot_text
python3 tests/e2e.py keyboard_irq
```
