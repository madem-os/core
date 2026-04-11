/*
 * x86 Exception Registration And Dispatch
 *
 * This file installs the current CPU exception handlers and owns the first
 * fatal page-fault reporting path. For now the kernel handles page faults by
 * printing the fault address and error code, then halting instead of silently
 * resetting through a triple fault.
 *
 * Responsibilities in this file:
 * - install the page-fault handler in the IDT
 * - decode the page-fault stack frame passed from assembly
 * - read CR2 and report the fault through the panic path
 *
 * This stage does not yet implement recovery, demand paging, or a general
 * exception-dispatch framework.
 */

#include <stdint.h>

#include "arch/x86/exceptions.h"
#include "arch/x86/idt.h"
#include "arch/x86/lowlevel.h"
#include "kernel/panic.h"

#define PAGE_FAULT_VECTOR 14u

struct page_fault_frame {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
};

extern void page_fault_stub(void);

void page_fault_handle(const struct page_fault_frame *frame) {
    panic_write("page fault: cr2=");
    panic_write_hex32(x86_read_cr2());
    panic_write(" error=");
    panic_write_hex32(frame->error_code);
    panic_write(" eip=");
    panic_write_hex32(frame->eip);
    panic_write("\n");
    x86_halt_forever();
}

void exceptions_init(void) {
    idt_set_gate(PAGE_FAULT_VECTOR, (uint32_t)page_fault_stub);
}
