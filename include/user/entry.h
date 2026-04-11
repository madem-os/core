/*
 * Userspace Entry Declaration
 *
 * This header declares the user C entry point reached from the userspace entry
 * stub inside the separately linked user ELF image. The kernel no longer jumps
 * to this symbol directly; it transfers control to the ELF entry point, which
 * then sets up user segments and calls `main(argc, argv)`.
 */

#ifndef USER_ENTRY_H
#define USER_ENTRY_H

int main(int argc, char **argv);

#endif
