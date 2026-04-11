/*
 * First Built-In Userspace Program
 *
 * This file provides the first ring 3 application used to prove the minimal
 * usermode vertical slice. It writes a startup line, then repeatedly reads one
 * cooked line from stdin and writes it back to stdout.
 */

#include "user/entry.h"
#include "user/section.h"
#include "user/syscall.h"

static const char user_mode_ready[] USER_RODATA = "user mode ready\n";

USER_TEXT void user_main(void) {
    char buf[128];

    (void)write(1, user_mode_ready, (int)(sizeof(user_mode_ready) - 1u));

    for (;;) {
        int count;

        count = read(0, buf, (int)sizeof(buf));
        if (count > 0) {
            (void)write(1, buf, count);
        }
    }
}
