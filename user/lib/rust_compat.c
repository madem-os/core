/*
 * Minimal Rust Userspace Compatibility Symbols
 *
 * These symbols satisfy compiler-emitted libc-style helpers that freestanding
 * Rust user apps may reference.
 */

#include <stddef.h>

void *memset(void *dst, int value, size_t len) {
    unsigned char *bytes;
    size_t index;

    bytes = (unsigned char *)dst;
    for (index = 0; index < len; index++) {
        bytes[index] = (unsigned char)value;
    }

    return dst;
}
