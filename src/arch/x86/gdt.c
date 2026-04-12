/*
 * x86 GDT And TSS Setup
 *
 * This file owns the runtime GDT/TSS install performed by the higher-half
 * kernel after early boot. The bootloader's temporary low-memory GDT is
 * sufficient to reach C, but once the low identity alias is removed the CPU
 * must use descriptor tables mapped through the higher half as well.
 */

#include <stdint.h>

#include "arch/x86/gdt.h"
#include "arch/x86/lowlevel.h"

#define KERNEL_CODE_SELECTOR 0x08u
#define KERNEL_DATA_SELECTOR 0x10u
#define USER_CODE_SELECTOR   0x18u
#define USER_DATA_SELECTOR   0x20u
#define TSS_SELECTOR         0x28u
#define KERNEL_STACK_TOP     0xC0090000u

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt_selector;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct gdt_entry kernel_gdt[6];
static struct gdt_ptr kernel_gdt_ptr;
static struct tss_entry kernel_tss;

static void gdt_set_entry(
    struct gdt_entry *entry,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t granularity
) {
    entry->limit_low = (uint16_t)(limit & 0xFFFFu);
    entry->base_low = (uint16_t)(base & 0xFFFFu);
    entry->base_mid = (uint8_t)((base >> 16) & 0xFFu);
    entry->access = access;
    entry->granularity = (uint8_t)(((limit >> 16) & 0x0Fu) | (granularity & 0xF0u));
    entry->base_high = (uint8_t)((base >> 24) & 0xFFu);
}

void gdt_init(void) {
    uint32_t tss_base;
    uint32_t tss_limit;

    gdt_set_entry(&kernel_gdt[0], 0, 0, 0, 0);
    gdt_set_entry(&kernel_gdt[1], 0, 0xFFFFFu, 0x9Au, 0xCFu);
    gdt_set_entry(&kernel_gdt[2], 0, 0xFFFFFu, 0x92u, 0xCFu);
    gdt_set_entry(&kernel_gdt[3], 0, 0xFFFFFu, 0xFAu, 0xCFu);
    gdt_set_entry(&kernel_gdt[4], 0, 0xFFFFFu, 0xF2u, 0xCFu);

    for (tss_base = 0; tss_base < sizeof(kernel_tss); tss_base++) {
        ((uint8_t *)&kernel_tss)[tss_base] = 0;
    }
    kernel_tss.esp0 = KERNEL_STACK_TOP;
    kernel_tss.ss0 = KERNEL_DATA_SELECTOR;
    kernel_tss.iomap_base = sizeof(kernel_tss);

    tss_base = (uint32_t)(uintptr_t)&kernel_tss;
    tss_limit = (uint32_t)sizeof(kernel_tss) - 1u;
    gdt_set_entry(&kernel_gdt[5], tss_base, tss_limit, 0x89u, 0x00u);

    kernel_gdt_ptr.limit = (uint16_t)(sizeof(kernel_gdt) - 1u);
    kernel_gdt_ptr.base = (uint32_t)(uintptr_t)&kernel_gdt;

    x86_load_gdt(&kernel_gdt_ptr);
    x86_load_task_register(TSS_SELECTOR);
}
