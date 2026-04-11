/*
 * First Built-In Userspace Program
 *
 * This file provides the first ring 3 application used to prove the minimal
 * usermode vertical slice. It writes a startup line, then repeatedly reads one
 * cooked line from stdin and writes it back to stdout.
 */

#include "user/entry.h"
#include "user/syscall.h"

void user_main(void) {
    char buf[128];

    (void)write(1, "user mode ready\n", 16);

    for (;;) {
        int count;

        count = read(0, buf, (int)sizeof(buf));
        if (count > 0) {
            (void)write(1, buf, count);
        }
    }
}
