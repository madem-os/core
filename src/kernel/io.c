/*
 * Kernel I/O Dispatch
 *
 * This file implements the minimal fd-oriented read/write API used by the
 * kernel before syscall entry and user mode are introduced. It routes through
 * the current process and dispatches TTY-backed descriptors to the TTY layer.
 *
 * Responsibilities in this file:
 * - validate fd-oriented kernel I/O requests
 * - consult the current process descriptor table
 * - dispatch stdin/stdout/stderr operations to the TTY layer
 *
 * The first implementation only supports TTY-backed file descriptors and
 * returns `-1` for invalid or unsupported operations.
 */

#include <stddef.h>

#include "kernel/io.h"
#include "kernel/process.h"
#include "tty/tty.h"

static struct file_descriptor *kernel_get_fd(int fd) {
    struct process *process;

    if (fd < 0 || fd >= 3) {
        return NULL;
    }

    process = process_current();
    if (process == NULL) {
        return NULL;
    }

    return &process->fds[fd];
}

int kread(int fd, char *buf, int len) {
    struct file_descriptor *descriptor;

    if (buf == NULL || len <= 0 || fd != 0) {
        return -1;
    }

    descriptor = kernel_get_fd(fd);
    if (descriptor == NULL || descriptor->kind != FD_KIND_TTY) {
        return -1;
    }

    return tty_read((struct tty *)descriptor->object, buf, len);
}

int kwrite(int fd, const char *buf, int len) {
    struct file_descriptor *descriptor;

    if (buf == NULL || len <= 0 || (fd != 1 && fd != 2)) {
        return -1;
    }

    descriptor = kernel_get_fd(fd);
    if (descriptor == NULL || descriptor->kind != FD_KIND_TTY) {
        return -1;
    }

    return tty_write((struct tty *)descriptor->object, buf, len);
}
