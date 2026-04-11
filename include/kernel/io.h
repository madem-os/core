/*
 * Kernel I/O Interface
 *
 * This header declares the minimal fd-oriented read/write interface used by
 * the kernel before user mode exists. It routes through the current process
 * and its stdio descriptor table instead of calling TTY services directly.
 *
 * Responsibilities in this header:
 * - expose kernel-side `read` and `write` style entry points
 * - keep the syscall-facing ABI shape stable before syscalls are added
 *
 * The first stage only supports TTY-backed stdin/stdout/stderr.
 */

#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include <stdint.h>

int kread(int fd, char *buf, int len);
int kwrite(int fd, const char *buf, int len);
int kwrite_hex32(int fd, uint32_t value);

#endif
