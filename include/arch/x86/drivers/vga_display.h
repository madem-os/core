/*
 * x86 VGA Display Backend
 *
 * This header declares the VGA text-mode implementation of the generic
 * display abstraction used by the new text console.
 *
 * Responsibilities in this header:
 * - expose a single initializer for wiring a display object to VGA text mode
 * - keep VGA-specific constants and callbacks hidden inside the backend
 * - make the x86 ownership of this backend explicit in the include path
 *
 * Higher layers should depend on `struct display` and the text console API,
 * not on VGA memory addresses or cursor-port details directly.
 */

#ifndef ARCH_X86_DRIVERS_VGA_DISPLAY_H
#define ARCH_X86_DRIVERS_VGA_DISPLAY_H

#include "console/display.h"

void init_vga_display(struct display *display, void *ctx);

#endif
