/*
 * Kernel Process State
 *
 * This file owns the minimal process and file-descriptor state required for
 * the first usermode-preparation stage. It tracks one current process and
 * wires stdin/stdout/stderr to a TTY object.
 *
 * Responsibilities in this file:
 * - initialize a minimal process object
 * - bind stdio descriptors to a TTY-backed endpoint
 * - store and return the current process pointer
 *
 * This file intentionally does not implement scheduling, context switching, or
 * a general open-file model. Those belong to later milestones.
 */

#include <stddef.h>

#include "kernel/process.h"

static struct process *current_process;

void process_init(struct process *process) {
    int index;

    for (index = 0; index < 3; index++) {
        process->fds[index].kind = FD_KIND_NONE;
        process->fds[index].object = NULL;
    }
}

void process_set_tty_stdio(struct process *process, void *tty) {
    int index;

    for (index = 0; index < 3; index++) {
        process->fds[index].kind = FD_KIND_TTY;
        process->fds[index].object = tty;
    }
}

struct process *process_current(void) {
    return current_process;
}

void process_set_current(struct process *process) {
    current_process = process;
}
