#include "kernel/process.h"
#include "tty/tty.h"
#include "console/text_console.h"
#include "console/display.h"
#include "kernel/vm.h"
    
struct tty *get_global_tty(void);
struct text_console *get_global_text_console(void);
struct display *get_global_display(void);
struct process *get_kernel_process(void);
struct vm_user_image *get_initial_user_image(void);
