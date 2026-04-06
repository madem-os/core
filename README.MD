# MademOS

MademOS is a hobby x86 operating system project built from scratch.

The project currently contains:

- a boot sector and stage-2 loader written in NASM
- a tiny 32-bit freestanding kernel written in C
- a linker script for placing the kernel at `0x00100000`
- a `build.sh` script that builds a bootable disk image and runs it in QEMU

At the moment, the kernel boots into protected mode and prints a message to VGA text mode.

## Project Layout

- `bootS.asm`: boot sector + stage-2 loader
- `kernel.c`: simple kernel entry and VGA text output
- `link.ld`: ELF linker script for the kernel
- `build.sh`: build and run script
- `bin/`: generated build artifacts such as `disk.img`, `kernel.elf`, and `kernel.bin`

## Build Output

Running `build.sh` creates all generated files under `bin/`:

- `bin/bootS.bin`
- `bin/kernel.o`
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

Example manual run:

```bash
qemu-system-i386 -m 512 -drive file=bin/disk.img,format=raw
```

## E2E Test

There is a simple end-to-end VGA test under `tests/e2e_vga.py`.

It does the following:

1. runs `./build.sh --build-only`
2. boots `bin/disk.img` in headless QEMU
3. reads the VGA text buffer through the QEMU monitor
4. asserts that the expected text appears

Run it with:

```bash
python3 tests/e2e_vga.py
```
