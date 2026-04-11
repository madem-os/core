/*
 * Kernel Panic Interface
 *
 * This header declares the minimal panic-output path used when the kernel hits
 * unrecoverable faults such as a page fault without a recovery strategy.
 *
 * Responsibilities in this header:
 * - allow the kernel to register the active text console for panic output
 * - expose simple string and hex writers that avoid the normal fd/TTY path
 * - expose a terminal panic helper that halts the machine after printing
 *
 * This is intentionally not a generic logging framework. It exists so fatal
 * exception paths can report useful state without depending on process or TTY
 * dispatch being healthy.
 */

#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include <stdint.h>

struct text_console;

/* Registers the initialized text console used for panic output. */
void panic_set_console(struct text_console *console);
/* Writes a raw byte string through the panic output path. */
void panic_write(const char *text);
/* Writes a 32-bit value in 0xXXXXXXXX hexadecimal form through the panic path. */
void panic_write_hex32(uint32_t value);
/* Prints a final panic line and halts the CPU forever. */
void panic_die(const char *message);

#endif
