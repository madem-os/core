/*
 * First Built-In Userspace Program
 *
 * This file provides the first ring 3 application used to prove the minimal
 * usermode vertical slice. It writes a startup line, then repeatedly reads one
 * cooked line from stdin and writes it back to stdout.
 */

#include "user/entry.h"
#include "user/syscall.h"

static const char user_mode_ready[] = "Welcome to shell!\n";

int main(int argc, char **argv) {
    char buf[128];

    (void)argc;
    (void)argv;

    (void)write(1, user_mode_ready, (int)(sizeof(user_mode_ready) - 1u));

    for (;;) {
        int count;
        count = read(0, buf, (int)sizeof(buf));
        if (count > 0) {    
            (void)execve("echo_line");
        }
    }

    return 0;
}
