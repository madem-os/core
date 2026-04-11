/*
 * Userspace Entry Declaration
 *
 * This header declares the fixed built-in userspace entry point used by the
 * first usermode milestone. The kernel transfers control to this symbol after
 * preparing the initial user stack and stdio wiring.
 */

#ifndef USER_ENTRY_H
#define USER_ENTRY_H

void user_main(void);

#endif
