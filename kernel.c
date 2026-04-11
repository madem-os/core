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

#include "arch/x86/idt.h"
#include "arch/x86/irq.h"
#include "arch/x86/pic.h"
#include "arch/x86/drivers/keyboard.h"
#include "arch/x86/drivers/vga_display.h"
#include "arch/x86/syscall.h"
#include "arch/x86/usermode.h"
#include "console/display.h"
#include "console/text_console.h"
#include "input/input.h"
#include "kernel/io.h"
#include "kernel/process.h"
#include "tty/tty.h"
#include "user/entry.h"

#if defined(__ELF__)
#define KERNEL_ENTRY_SECTION __attribute__((section(".text.entry")))
#else
#define KERNEL_ENTRY_SECTION
#endif

__attribute__((naked, used)) KERNEL_ENTRY_SECTION
void kernel_entry(void) {
    __asm__ volatile (
        "cli\n"
        "cld\n"
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
        "mov $0x00090000, %esp\n"
        "call kmain\n"
        "1:\n"
        "hlt\n"
        "jmp 1b\n"
    );
}


struct tty global_tty;
struct text_console global_text_console;
struct display vga_display;
struct process kernel_process;
static uint8_t user_stack[4096];

char buf[256];

void kmain(void) {

    idt_init();
    pic_init();
    irq_init();
    syscall_init();
    input_init();
    keyboard_init(input_handle_scancode);

    init_vga_display(&vga_display, 0);
    text_console_init(&global_text_console, &vga_display);
    tty_init(
        &global_tty,
        input_read_byte_blocking,
        text_console_tty_write,
        &global_text_console
    );
    process_init(&kernel_process);
    process_set_tty_stdio(&kernel_process, &global_tty);
    kernel_process.entry_point = (uintptr_t)user_main;
    kernel_process.user_stack_top = (uintptr_t)(user_stack + sizeof(user_stack));
    process_set_current(&kernel_process);
    
    asm volatile("sti");

    kwrite(1, "welcome to Madem-OS!\n", 21);
    x86_enter_usermode(
        (uint32_t)kernel_process.entry_point,
        (uint32_t)kernel_process.user_stack_top
    );
}
