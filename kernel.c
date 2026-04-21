/*
 * Kernel Entry And Orchestration
 *
 * This translation unit is the high-level entry point for the 32-bit kernel.
 * It is entered from the boot assembly after protected mode is enabled and
 * owns the top-level sequencing of subsystem initialization.
 *
 * Responsibilities in this file:
 * - define the first C entry point reached from assembly
 * - establish the early kernel stack and jump into regular C code
 * - initialize the core runtime subsystems in a readable order
 * - keep the main kernel loop simple and easy to inspect
 *
 * This file should not become the implementation home for architecture code,
 * interrupt controller setup, raw device I/O, or console internals. Those
 * belong to the dedicated modules under arch/, drivers/, and console/.
 *
 * The current kernel flow is intentionally small: initialize interrupts,
 * enable the keyboard driver, print boot status, and halt while waiting for
 * IRQs. Future orchestration can grow here as more subsystems appear.
 */

#include <stdint.h>

#include "arch/x86/exceptions.h"
#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"
#include "arch/x86/irq.h"
#include "arch/x86/lowlevel.h"
#include "arch/x86/paging.h"
#include "arch/x86/pic.h"
#include "arch/x86/drivers/keyboard.h"
#include "arch/x86/drivers/vga_display.h"
#include "arch/x86/syscall.h"
#include "arch/x86/usermode.h"
#include "console/display.h"
#include "console/text_console.h"
#include "input/input.h"
#include "kernel/io.h"
#include "kernel/panic.h"
#include "kernel/process.h"
#include "kernel/user_programs.h"
#include "kernel/vm.h"
#include "tty/tty.h"
#include "kernel/singletones.h"
#include "kernel/exec.h"

#if defined(__ELF__)
#define KERNEL_ENTRY_SECTION __attribute__((section(".text.entry")))
#else
#define KERNEL_ENTRY_SECTION
#endif

extern void kernel_entry(void);

void kmain(void) {
    gdt_init();
    paging_unmap_bootstrap_low_alias();
    idt_init();
    exceptions_init();
    pic_init();
    irq_init();
    syscall_init();
    input_init();
    keyboard_init(input_handle_scancode);
    x86_enable_interrupts();

    init_vga_display(get_global_display(), 0);
    text_console_init(get_global_text_console(), get_global_display());
    panic_set_console(get_global_text_console());
    tty_init(
        get_global_tty(),
        input_read_byte_blocking,
        text_console_tty_write,
        get_global_text_console()
    );

    struct tty* gtty = get_global_tty();
    gtty->write_output(gtty->output_ctx, "Welcome to Madem-OS!\n", 21);
    gtty->write_output(gtty->output_ctx, "Kernel_entry=", 13);
    char hex_buffer[10];
    int_to_hex32((uint32_t)(uintptr_t)kernel_entry, hex_buffer);
    gtty->write_output(gtty->output_ctx, hex_buffer, 10);
    gtty->write_output(gtty->output_ctx, "\n", 1);

    execve("shell");
}
