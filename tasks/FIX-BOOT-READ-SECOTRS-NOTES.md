# Kernel Print Regression Investigation

## Summary

Adding an unused `dummy_func()` to `kernel.c` made the kernel stop showing the printed message, but the new function was not the actual bug.

The real issue was in the stage-2 disk loader in `bootS.asm`. The loader was not correctly reading the full kernel image from disk. It only behaved well enough while the important kernel code and string data happened to live in the first 512-byte sector of `kernel.bin`.

Once the extra function changed code layout, `kmain()` and the string literal moved into later sectors, and the broken loader path became visible.

## Symptom

Before `dummy_func()` was added:

- `printf("hello ...")` appeared to work.
- The kernel seemed to boot normally.

After `dummy_func()` was added:

- The screen stayed blank.
- It looked like the kernel stopped working.

## Initial Hypotheses

Possible causes considered:

- linker section misalignment
- entry point moved unexpectedly
- stack corruption
- `.rodata` or `.bss` layout changed
- the loader no longer copying the full kernel into memory

## What Was Checked

### 1. Kernel ELF layout

The kernel ELF was inspected after the change.

Relevant symbols:

- `kernel_entry` at `0x00100000`
- `dummy_func` at `0x001001d0`
- `kmain` at `0x001002b0`
- string literal at `0x001002d0`
- `character_ptr` at `0x001002e0`

This showed the extra function did change layout, but the ELF itself was still valid.

### 2. Flat binary contents

`kernel.bin` was checked directly.

Important result:

- the bytes for `kmain` were present at offset `0x2b0`
- the string `"hello yoti!"` was present at offset `0x2d0`

So the linker and `objcopy` output were correct on disk.

### 3. Guest memory after boot

QEMU monitor memory inspection showed the failure clearly.

Before the bootloader fix:

- memory at `0x001002b0` did **not** match the `kmain` bytes from `kernel.bin`
- memory at `0x001002d0` did **not** contain `"hello yoti!"`
- VGA memory at `0x000b8000` remained blank

That meant the kernel image in RAM no longer matched the kernel image on disk.

### 4. CPU state

CPU register inspection showed execution had gone wrong because the loaded image was corrupted/incomplete in RAM, not because of a section alignment problem or intended control flow.

## Root Cause

The bug was in the ATA PIO read logic in `bootS.asm`.

The old code:

- programmed a large multi-sector read
- waited for readiness once
- then streamed a large block of words from port `0x1f0`

That is not a correct way to read the full kernel image sector-by-sector in this setup.

In practice, the first sector was available reliably, but later sectors were not being handled correctly. That created a hidden dependency:

- if all important code/data stayed in sector 1, the kernel looked fine
- once `dummy_func()` pushed `kmain` and `.rodata` into sector 2, boot broke

So the added dummy function only exposed the real bug.

## Why It Looked Like A Layout Bug

It was reasonable to suspect alignment or stack issues because:

- the regression happened after adding an unused function
- symbol addresses changed
- the kernel stopped printing without any direct logic change

But the linked image itself was still valid. The failure appeared only after the bootloader copied the image into RAM incorrectly.

## Fix

The stage-2 loader in `bootS.asm` was rewritten to load the kernel one sector at a time.

New behavior:

1. Set LBA to the kernel start (`64`)
2. Set the destination buffer to `0x00100000`
3. For each sector:
   - issue a read request for exactly 1 sector
   - wait for the device to become ready
   - read exactly 256 words from port `0x1f0`
   - advance the LBA by 1
   - advance the destination buffer by 512 bytes
4. Repeat until the full kernel region is loaded

This removed the assumption that one readiness check was enough for a large multi-sector transfer.

## Files Changed

### `bootS.asm`

Changed the kernel-loading logic:

- removed the old bulk read path
- removed the old `read_harddisk_init` logic
- changed `read_sectors` to request 1 sector at a time
- changed `load_to_ram` to read exactly 256 words per sector
- added an explicit loop over all kernel sectors

### `kernel.c`

No functional fix was needed for this regression itself. The newly added `dummy_func()` was only useful because it exposed the loader bug by changing layout.

## Verification After Fix

After the loader change:

- `kmain` bytes in guest RAM matched the bytes in `kernel.bin`
- the string at `0x001002d0` was present in RAM
- VGA memory contained the expected characters
- the screen displayed `hello yoti!`

## Final Conclusion

The regression was **not** caused by:

- linker section alignment
- the kernel entry point
- the kernel stack
- the unused function itself

The regression was caused by a broken stage-2 disk loader that failed to load later sectors of the kernel correctly. The extra function only changed layout enough to expose that bug.

## Why The Previous Loader Code Was Wrong

This is the old loader logic that caused the problem:

```asm
sector_count dw 0
lba_address dd 0
buffer_address dd 0
number_of_bytes dd 0

read_kernel:
mov dword [lba_address], 64
mov word [sector_count], 2048
mov dword [buffer_address], 0x00100000
call read_harddisk_init
ret

read_sectors:
push eax
push edx
mov al, 0
mov dx, 0x1f2
out dx, al
inc dx
mov eax, [lba_address]
out dx, al
inc dx
shr eax, 8
out dx, al
inc dx
shr eax, 8
out dx, al
inc dx
shr eax, 8
and al, 0x0f
or al, 0xe0
out dx, al
inc dx
mov al, 0x20
out dx, al

wait_bsy:
in al, dx
test al, 0x80
jnz wait_bsy

wait_drq:
in al, dx
test al, 0x08
jz wait_drq

pop edx
pop eax
ret

load_to_ram:
push edx
push edi
push ecx
push eax
mov dx, 0x1f0
mov edi, [buffer_address]
mov ecx, [number_of_bytes]

load_loop:
in ax, dx
mov [edi], ax
add edi, 2
loop load_loop

pop eax
pop ecx
pop edi
pop edx
ret

read_harddisk_init:
push ecx
xor ecx, ecx
mov cl, 8
mov word [sector_count], 256
mov dword [number_of_bytes], 65536

init_loop:
call read_sectors
add dword [lba_address], 256
call load_to_ram
add dword [buffer_address], 131072
loop init_loop

pop ecx
ret
```

### The Core Mistake

The old code mixed up these two ideas:

- telling the disk to prepare sectors
- actually reading one sector's worth of data from the data port

For ATA PIO reads, each sector is `512` bytes, which means:

- `256` reads from port `0x1f0`
- then the drive must prepare the next sector
- then software must wait again before reading the next sector

The buggy code did not follow that contract cleanly.

### Problem 1: It Requested The Read In A Strange Way

Inside `read_sectors`, the code did:

```asm
mov al, 0
mov dx, 0x1f2
out dx, al
...
mov al, 0x20
out dx, al
```

Port `0x1f2` is the ATA sector count register.

Writing `0` there does **not** mean "read zero sectors safely and I will manage the rest manually". In ATA, a value of `0` means `256` sectors.

So every call to `read_sectors` was actually asking the drive for a `256-sector` transfer.

### Problem 2: It Waited Only Once Per 256-Sector Request

After issuing that request, the code did:

```asm
wait_bsy:
in al, dx
test al, 0x80
jnz wait_bsy

wait_drq:
in al, dx
test al, 0x08
jz wait_drq
```

That wait is only enough to confirm that **the current sector** is ready to be transferred.

It is not enough to prove that all `256` sectors are immediately available for one giant uninterrupted stream from `0x1f0`.

The drive-device protocol is effectively:

1. host issues a read command
2. drive prepares one sector
3. drive raises `DRQ`
4. host reads exactly `256` words
5. drive prepares the next sector
6. host must observe readiness again

The old code skipped step 6 for sectors 2..256.

### Problem 3: It Read 65536 Bytes As One Continuous Stream

This part is the real failure:

```asm
mov ecx, [number_of_bytes]
...
load_loop:
in ax, dx
mov [edi], ax
add edi, 2
loop load_loop
```

`number_of_bytes` was set to `65536`, so the code executed `65536` port reads.

But each `in ax, dx` reads only one word, which is `2` bytes.

That means the code actually tried to consume:

- `65536` words
- `131072` bytes
- which is `256` sectors

So the variable name was misleading. It was not counting bytes in the loop. It was counting iterations, and each iteration read a 16-bit word.

That made the bug harder to notice.

### Problem 4: Why It Seemed To Work Before

Even though the protocol was wrong, the kernel still appeared to boot when the important data lived very early in the image.

That happened because:

- `kernel_entry`
- early code
- some helper functions
- and the string data

all happened to be inside the first sector or very close to the front of the image.

So even a partially-correct transfer was enough for the kernel to start and print.

After `dummy_func()` was added:

- `kmain` moved to `0x001002b0`
- the string moved to `0x001002d0`
- both landed in the second sector of `kernel.bin`

Now the broken multi-sector logic mattered, because the first sector was no longer enough.

### What Was Seen In Practice

Before the fix:

- memory at `0x00100000` contained the first sector correctly
- memory later in the kernel image did not match the file on disk
- `kmain` was corrupted in RAM
- the string literal was missing in RAM
- VGA output stayed blank

That is exactly what you would expect if the loader handled sector 1 correctly and mishandled later sectors.

### Why The New Loader Works

The fixed loader does this instead:

1. request exactly `1` sector
2. wait for the drive to signal readiness
3. read exactly `256` words
4. advance the destination pointer by `512`
5. advance the LBA by `1`
6. repeat

That matches the ATA PIO transfer model directly and removes the broken assumption that one readiness check can cover a giant multi-sector stream.
