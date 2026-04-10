/*
 * x86 Port I/O Interface
 *
 * This header exposes the raw x86 port access helpers used by architecture
 * support code and low-level drivers that must communicate through I/O ports.
 *
 * Responsibilities in this header:
 * - declare byte and word output helpers
 * - declare byte input helpers
 * - declare the tiny I/O wait primitive used by controller setup code
 *
 * This interface is intentionally small and does not define device-specific
 * register layouts or policy. Consumers should layer their own semantics on
 * top of these primitives.
 */

#ifndef ARCH_X86_PORTS_H
#define ARCH_X86_PORTS_H

#include <stdint.h>

void write_port_byte(uint16_t port, uint8_t value);
uint8_t read_port_byte(uint16_t port);
void write_port_word(uint16_t port, uint16_t value);
void io_wait(void);

#endif
