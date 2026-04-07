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

static int shift_pressed = 0;
static int ctrl_pressed = 0;
static bool used = false;

void process_scancode(uint8_t scancode) {
    if (scancode == 0x2A) shift_pressed = 1;      // Shift make
    else if (scancode == 0xAA) shift_pressed = 0;  // Shift break
    else if (scancode == 0x1D) ctrl_pressed = 1;   // Ctrl make
    else if (scancode == 0x9D) ctrl_pressed = 0;   // Ctrl break
    else {
        if (!used) {
        char ascii = get_ascii(scancode);
        if (shift_pressed && ascii >= 'a' && ascii <= 'z') {
            ascii -= 32;  // Convert to uppercase
        }
        print_char((uint8_t)ascii);
        used = true;
        }
        else {
            used = false;
        }
    }
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



