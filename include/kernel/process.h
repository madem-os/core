/*
 * Kernel Process Interface
 *
 * This header declares the minimal process and file-descriptor state used by
 * the first usermode preparation stage. It models only enough kernel-owned
 * process state to route stdin/stdout/stderr through the active TTY.
 *
 * Responsibilities in this header:
 * - define the minimal file descriptor entry shape
 * - define the minimal process object used by the kernel
 * - expose helpers for stdio wiring and current-process tracking
 *
 * This is intentionally not a full scheduler or VFS interface. It exists to
 * give the kernel a process-shaped boundary before ring 3 and syscalls are
 * introduced.
 */

#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

enum fd_kind {
    FD_KIND_NONE = 0,
    FD_KIND_TTY = 1
};

struct file_descriptor {
    enum fd_kind kind;
    void *object;
};

struct process {
    struct file_descriptor fds[3];
};

void process_init(struct process *process);
void process_set_tty_stdio(struct process *process, void *tty);
struct process *process_current(void);
void process_set_current(struct process *process);

#endif
