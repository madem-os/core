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
#include "arch/x86/idt.h"
#include "arch/x86/irq.h"
#include "arch/x86/lowlevel.h"
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

#if defined(__ELF__)
#define KERNEL_ENTRY_SECTION __attribute__((section(".text.entry")))
#else
#define KERNEL_ENTRY_SECTION
#endif

extern void kernel_entry(void);

struct tty global_tty;
struct text_console global_text_console;
struct display vga_display;
struct process kernel_process;

char buf[256];

void kmain(void) {
    const struct user_program *initial_program;
    struct vm_user_image initial_user_image;

    idt_init();
    exceptions_init();
    pic_init();
    irq_init();
    syscall_init();
    input_init();
    keyboard_init(input_handle_scancode);

    init_vga_display(&vga_display, 0);
    text_console_init(&global_text_console, &vga_display);
    panic_set_console(&global_text_console);
    tty_init(
        &global_tty,
        input_read_byte_blocking,
        text_console_tty_write,
        &global_text_console
    );
    process_init(&kernel_process);
    process_set_tty_stdio(&kernel_process, &global_tty);
    process_set_current(&kernel_process);
    initial_program = user_program_find("echo_line");
    if (
        initial_program == NULL ||
        vm_user_image_from_elf(
            &initial_user_image,
            initial_program->elf_start,
            (size_t)(initial_program->elf_end - initial_program->elf_start)
        ) != 0 ||
        vm_init_process(
            &kernel_process,
            vm_default_runtime(),
            &initial_user_image
        ) != 0
    ) {
        kwrite(1, "vm init failed\n", 15);
        for (;;) {
        }
    }

    x86_enable_interrupts();

    kwrite(1, "welcome to Madem-OS!\n", 21);
    kwrite(1, "kernel_entry=", 13);
    kwrite_hex32(1, (uint32_t)(uintptr_t)kernel_entry);
    kwrite(1, "\n", 1);
    vm_activate_process(&kernel_process, vm_default_runtime());
    x86_enter_usermode(
        (uint32_t)kernel_process.entry_point,
        (uint32_t)kernel_process.user_stack_top
    );
}
