// kernel.c
#include "mos-stdio.h"
#include <stdint.h>
#include <stdbool.h>

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
    print_line("welcome to Madem-OS!");
    print_line("mode set to free texting");
    printf("Madem-OS:/");

    while (1) {
        uint8_t character = input();
        process_scancode(character);        
    }
}



