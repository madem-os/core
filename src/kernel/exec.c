#include "console/display.h"
#include "console/text_console.h"
#include "input/input.h"
#include "kernel/io.h"
#include "kernel/panic.h"
#include "kernel/process.h"
#include "kernel/user_programs.h"
#include "kernel/vm.h"
#include "tty/tty.h"
#include "kernel/singletones.h"
#include "arch/x86/usermode.h"

void execve(const char *program_name) {
    const struct user_program *initial_program;
    struct vm_runtime *runtime;

    process_init(get_kernel_process());
    process_set_tty_stdio(get_kernel_process(), get_global_tty());
    process_set_current(get_kernel_process());
    runtime = vm_prepare_next_runtime();
    initial_program = user_program_find(program_name);
    if (
        runtime == NULL ||
        initial_program == NULL ||
        vm_user_image_from_elf(
            get_initial_user_image(),
            initial_program->elf_start,
            (size_t)(initial_program->elf_end - initial_program->elf_start)
        ) != 0 ||
        vm_init_process(
            get_kernel_process(),
            runtime,
            get_initial_user_image()
        ) != 0
    ) {
        kwrite(1, "vm init failed\n", 15);
        for (;;) {
        }
    }
    vm_activate_process(get_kernel_process(), runtime);
    vm_set_active_runtime(runtime);
    x86_enter_usermode(
        (uint32_t)get_kernel_process()->entry_point,
        (uint32_t)get_kernel_process()->user_stack_top
    );
}
