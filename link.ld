/* link.ld */
ENTRY(kernel_entry)

KERNEL_PHYS_BASE = 0x00100000;
KERNEL_VIRT_BASE = 0xC0000000;
USER_TEXT_BASE = 0x00400000;

PHDRS
{
    text PT_LOAD FLAGS(5);  /* R + X */
    data PT_LOAD FLAGS(6);  /* R + W */
    user PT_LOAD FLAGS(7);  /* R + W + X */
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

    . = USER_TEXT_BASE;

    . = ALIGN(16);
    .user : AT(LOADADDR(.bss) + SIZEOF(.bss))
    {
        _user_image_start = .;
        *(.user.text .user.text.*)
        *(.user.rodata .user.rodata.*)
        *(.user.data .user.data.*)
        *(.user.bss .user.bss.*)
        _user_image_end = .;
    } :user

    _user_image_entry = user_main;
    _user_image_load_start = LOADADDR(.user) + KERNEL_VIRT_BASE;
    _user_image_load_end = LOADADDR(.user) + SIZEOF(.user) + KERNEL_VIRT_BASE;

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.gnu*)
    }
}
