/* link.ld */
ENTRY(kernel_entry)

KERNEL_PHYS_BASE = 0x00100000;
KERNEL_VIRT_BASE = 0xC0000000;

PHDRS
{
    text PT_LOAD FLAGS(5);  /* R + X */
    data PT_LOAD FLAGS(6);  /* R + W */
}

SECTIONS
{
    . = KERNEL_PHYS_BASE + KERNEL_VIRT_BASE;

    . = ALIGN(16);
    .text.entry : AT(ADDR(.text.entry) - KERNEL_VIRT_BASE)
    {
        KEEP(*(.text.entry))
    } :text

    . = ALIGN(16);
    .text : AT(ADDR(.text) - KERNEL_VIRT_BASE)
    {
        *(.text .text.*)
    } :text

    . = ALIGN(16);
    .rodata : AT(ADDR(.rodata) - KERNEL_VIRT_BASE)
    {
        *(.rodata .rodata.*)
    } :text

    . = ALIGN(16);
    .data : AT(ADDR(.data) - KERNEL_VIRT_BASE)
    {
        *(.data .data.*)
    } :data

    . = ALIGN(16);
    .bss : AT(ADDR(.bss) - KERNEL_VIRT_BASE)
    {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.gnu*)
    }
}
