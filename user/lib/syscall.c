/*
 * Userspace Syscall Wrappers
 *
 * This file implements the minimal userspace wrappers over `int 0x80` used by
 * the first built-in user program.
 *
 * Responsibilities in this file:
 * - provide `read` and `write` wrappers
 * - marshal arguments into the register convention expected by the kernel
 */

#include "kernel/syscall.h"
#include "user/syscall.h"

extern int user_syscall3(int number, int arg1, int arg2, int arg3);

int read(int fd, char *buf, int len) {
    return user_syscall3(SYS_READ, fd, (int)buf, len);
}

int write(int fd, const char *buf, int len) {
    return user_syscall3(SYS_WRITE, fd, (int)buf, len);
}
