/* user/link.ld */
ENTRY(user_program_entry)

USER_TEXT_BASE = 0x00400000;

PHDRS
{
    app PT_LOAD FLAGS(7);  /* R + W + X */
}

SECTIONS
{
    . = USER_TEXT_BASE;

    . = ALIGN(16);
    .text :
    {
        *(.text .text.*)
    } :app

    . = ALIGN(16);
    .rodata :
    {
        *(.rodata .rodata.*)
    } :app

    . = ALIGN(16);
    .data :
    {
        *(.data .data.*)
    } :app

    . = ALIGN(16);
    .bss :
    {
        *(COMMON)
        *(.bss .bss.*)
    } :app

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.gnu*)
    }
}
