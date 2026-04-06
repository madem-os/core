/* link.ld */
ENTRY(kernel_entry)

PHDRS
{
    text PT_LOAD FLAGS(5);  /* R + X */
    data PT_LOAD FLAGS(6);  /* R + W */
}

SECTIONS
{
    . = 0x00100000;

    .text.entry : ALIGN(16)
    {
        KEEP(*(.text.entry))
    } :text

    .text : ALIGN(16)
    {
        *(.text .text.*)
    } :text

    .rodata : ALIGN(16)
    {
        *(.rodata .rodata.*)
    } :text

    .data : ALIGN(16)
    {
        *(.data .data.*)
    } :data

    .bss : ALIGN(16)
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
