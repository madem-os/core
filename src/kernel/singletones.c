#include "kernel/process.h"
#include "tty/tty.h"
#include "console/text_console.h"
#include "console/display.h"
#include "kernel/vm.h"

struct tty global_tty;
struct text_console global_text_console;
struct display vga_display;
struct process kernel_process;
struct vm_user_image initial_user_image;


struct tty *get_global_tty(void) {
    return &global_tty;
}

struct text_console *get_global_text_console(void) {
    return &global_text_console;
}

struct display *get_global_display(void) {
    return &vga_display;
}

struct process *get_kernel_process(void) {
    return &kernel_process;
}

struct vm_user_image *get_initial_user_image(void) {
    return &initial_user_image;
}