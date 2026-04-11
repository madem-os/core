/*
 * Userspace Entry Declaration
 *
 * This header declares the user C entry point reached from the userspace entry
 * stub inside the separately linked user ELF image. The kernel no longer jumps
 * to this symbol directly; it transfers control to the ELF entry point, which
 * then sets up user segments and calls `user_main()`.
 */

#ifndef USER_ENTRY_H
#define USER_ENTRY_H

void user_main(void);

#endif
