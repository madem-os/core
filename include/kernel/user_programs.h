/*
 * Built-In User Program Registry
 *
 * This header describes the generated registry of built-in user programs. Each
 * program is linked as its own ELF image, embedded into the kernel, and looked
 * up by name before the VM layer maps and starts it.
 */

#ifndef KERNEL_USER_PROGRAMS_H
#define KERNEL_USER_PROGRAMS_H

#include <stddef.h>
#include <stdint.h>

struct user_program {
    const char *name;
    const uint8_t *elf_start;
    const uint8_t *elf_end;
};

extern const struct user_program builtin_user_programs[];
extern const size_t builtin_user_program_count;

const struct user_program *user_program_find(const char *name);

#endif
