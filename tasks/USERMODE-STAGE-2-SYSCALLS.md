# Usermode Stage 2: Syscall Entry And Dispatch

## Scope

This document defines the second implementation slice toward user mode.

Stage 1 introduced:

- a minimal `struct process`
- a minimal fd table
- `kread`
- `kwrite`

This stage adds the first syscall boundary above that kernel I/O layer.

It introduces:

- an x86 syscall interrupt entry path
- a syscall dispatcher
- the first syscall ABI for `read` and `write`

This stage does not enter ring 3 yet. The first caller may still be kernel-side
test code or a temporary controlled entry path. The point of this stage is to
stabilize the syscall mechanism before the ring transition is added.

## Goal

After this stage:

- the kernel has a dedicated syscall interrupt vector
- an x86 trap entry can collect syscall arguments from registers
- the kernel can dispatch `read` and `write` syscalls to `kread` / `kwrite`
- the syscall ABI is explicit and documented
- the kernel build remains stable and testable

This creates the execution boundary that ring 3 code will later use.

## Non-Goals

This stage does not include:

- switching to ring 3
- creating a user stack
- loading a user program
- paging-based process isolation
- a full userspace library
- `exit` semantics unless we explicitly choose to add it now

## Recommended Mechanism

Use `int 0x80`.

Why:

- simple
- conventional for a 32-bit x86 teaching OS
- easy to inspect in assembly
- easy to connect to the IDT code you already have

There is no reason to start with `sysenter` or `syscall` here.

## Proposed Files

New files:

- `include/kernel/syscall.h`
- `src/kernel/syscall.c`
- `include/arch/x86/syscall.h`
- `src/arch/x86/syscall.c`

Likely existing files to change:

- `include/arch/x86/idt.h`
- `src/arch/x86/idt.c`
- `kernel.c`
- possibly `tests/run-unit.sh`

## High-Level Split

### Generic Kernel Side

`src/kernel/syscall.c` should own:

- syscall numbers
- generic syscall dispatch
- calling `kread` / `kwrite`

This layer should not know about:

- x86 trap frame details
- IDT entry format
- register-save assembly

### x86 Side

`src/arch/x86/syscall.c` should own:

- syscall vector number
- x86 assembly/C bridge
- extracting register arguments
- returning results in `eax`
- IDT registration of the syscall gate

This layer should not know about:

- TTY internals
- fd routing details
- process internals beyond calling the generic dispatcher

## Proposed Syscall ABI

### Vector

Use:

```c
#define SYSCALL_VECTOR 0x80
```

### Numbers

For this stage:

```c
enum syscall_number {
    SYS_READ = 0,
    SYS_WRITE = 1
};
```

Keep it that small for now.

We can add:

- `SYS_EXIT`
- `SYS_TCSETMODE`

in later slices.

### Calling Convention

Use:

- `eax` = syscall number
- `ebx` = arg1
- `ecx` = arg2
- `edx` = arg3
- return value in `eax`

So:

- `read(fd, buf, len)`:
  - `eax = SYS_READ`
  - `ebx = fd`
  - `ecx = buf`
  - `edx = len`

- `write(fd, buf, len)`:
  - `eax = SYS_WRITE`
  - `ebx = fd`
  - `ecx = buf`
  - `edx = len`

## Proposed Generic API

### `include/kernel/syscall.h`

```c
#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

enum syscall_number {
    SYS_READ = 0,
    SYS_WRITE = 1
};

int syscall_dispatch(int number, int arg1, int arg2, int arg3);

#endif
```

### `src/kernel/syscall.c`

Responsibilities:

- decode syscall number
- route supported syscalls to the kernel I/O layer
- return `-1` for unsupported syscalls

Expected behavior:

```c
int syscall_dispatch(int number, int arg1, int arg2, int arg3) {
    switch (number) {
        case SYS_READ:
            return kread(arg1, (char *)arg2, arg3);
        case SYS_WRITE:
            return kwrite(arg1, (const char *)arg2, arg3);
        default:
            return -1;
    }
}
```

This is the stable kernel-facing ABI entry point above `kread` / `kwrite`.

## Proposed x86 API

### `include/arch/x86/syscall.h`

```c
#ifndef ARCH_X86_SYSCALL_H
#define ARCH_X86_SYSCALL_H

void syscall_init(void);

#endif
```

### `src/arch/x86/syscall.c`

Responsibilities:

- install the syscall vector in the IDT
- provide the x86 syscall entry stub
- bridge register values into `syscall_dispatch`

Expected public function:

```c
void syscall_init(void);
```

Expected private pieces:

- one assembly stub used as the IDT target
- one C helper called by the stub

## Trap Entry Shape

The exact implementation is flexible, but the simplest model is:

1. CPU enters the syscall vector
2. assembly stub saves caller-visible registers
3. assembly stub passes `eax`, `ebx`, `ecx`, `edx` into a C helper
4. C helper calls `syscall_dispatch(...)`
5. return value is placed in `eax`
6. stub restores state and executes `iret`

For example conceptually:

```c
int x86_syscall_handle(int eax, int ebx, int ecx, int edx) {
    return syscall_dispatch(eax, ebx, ecx, edx);
}
```

The actual assembly/C interface can vary as long as it is simple and explicit.

## IDT Gate Requirements

The syscall vector must be installed in the IDT.

Important design requirement:

- the gate must later be callable from ring 3

That means the descriptor privilege level for the syscall gate should be chosen
with the future ring-3 transition in mind.

For a 32-bit x86 interrupt/trap gate used by user mode, this typically means:

- present bit set
- DPL = 3

Even if ring 3 is not active yet, setting this correctly now avoids a later ABI
change.

## Kernel Wiring

`kernel.c` should call:

```c
syscall_init();
```

after IDT setup and before interrupts are enabled.

Suggested init ordering:

```c
idt_init();
pic_init();
irq_init();
syscall_init();
```

The syscall path should be treated as part of the interrupt/exception entry
infrastructure, not as part of TTY or process setup.

## Testing Strategy

### Hosted Unit Tests

Add hosted tests for the generic dispatcher:

- `tests/unit/test_kernel_syscall.c`

These tests should cover:

1. `SYS_READ` dispatches to `kread`
2. `SYS_WRITE` dispatches to `kwrite`
3. unsupported syscall returns `-1`

Because hosted tests cannot execute the real x86 interrupt path, they should
test the generic kernel dispatcher only.

### Freestanding Verification

The x86 entry path should be verified by:

- successful kernel build
- a later e2e test once ring 3 exists

At this stage, the x86 syscall stub can be considered verified enough if:

- it builds
- it links
- it is installed into the IDT correctly

## Why This Stage Comes Before Ring 3

It reduces debugging complexity.

If ring 3 and syscall entry are added at the same time, failures can come from:

- GDT setup
- segment selectors
- stack setup
- privilege return path
- syscall IDT gate
- syscall argument handling
- dispatch logic

That is too much at once.

By doing syscall entry/dispatch first, the next stage only has to solve:

- creating the first user context
- transferring control to ring 3

The syscall handler itself is already in place.

## Suggested Implementation Order

1. add `include/kernel/syscall.h`
2. add `src/kernel/syscall.c`
3. add hosted dispatcher tests
4. add `include/arch/x86/syscall.h`
5. add `src/arch/x86/syscall.c`
6. install the syscall vector through `syscall_init()`
7. call `syscall_init()` from `kernel.c`
8. verify hosted tests and freestanding build

## Definition Of Done

This stage is done when:

- the generic syscall dispatcher exists
- `SYS_READ` and `SYS_WRITE` are defined
- the x86 syscall vector is installed in the IDT
- `kernel.c` initializes the syscall subsystem
- hosted unit tests cover dispatcher behavior
- the kernel still builds cleanly

