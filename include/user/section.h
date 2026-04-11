/*
 * Userspace Link Sections
 *
 * This header defines the section-placement attributes used to keep the built-
 * in ring-3 program in a dedicated low-VMA user image instead of folding it
 * into the higher-half kernel text and rodata.
 */

#ifndef USER_SECTION_H
#define USER_SECTION_H

/* Places user code in the low-VMA user image region emitted by the kernel link. */
#define USER_TEXT __attribute__((section(".user.text")))
/* Places user read-only data next to the copied low-VMA user image. */
#define USER_RODATA __attribute__((section(".user.rodata")))
/* Places user writable initialized data in the copied low-VMA user image. */
#define USER_DATA __attribute__((section(".user.data")))
/* Places user zero-initialized data in the copied low-VMA user image. */
#define USER_BSS __attribute__((section(".user.bss")))

#endif
