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
#include "console/console.h"
#include "arch/x86/drivers/keyboard.h"

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

void kmain(void) {
    console_write_line("welcome to Madem-OS!");

    idt_init();
    pic_init();
    irq_init();
    keyboard_init();

    asm volatile("sti");

    console_write_line("interrupts ready");

    while (1) {
        asm volatile("hlt");
    }
}
