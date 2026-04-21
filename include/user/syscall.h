/*
 * Userspace Syscall Wrappers
 *
 * This header declares the tiny userspace-facing syscall wrapper layer used by
 * the first built-in ring 3 application. It provides a libc-like shape without
 * introducing a full userspace runtime.
 *
 * Responsibilities in this header:
 * - expose `read` and `write` wrappers over `int 0x80`
 * - keep the first user application independent of kernel internals
 */

#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

int read(int fd, char *buf, int len);
int write(int fd, const char *buf, int len);
void execve(const char *program_name);

#endif
