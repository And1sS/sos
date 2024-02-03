ENTRY(main)

SECTIONS
{
    . = 0x1000;

    .text : {
        *(.entrypoint);
        *(.text)
    }

    .rodata : { *(.rodata) }

    .data : { *(.data) }

    .bss : { *(.bss) }

    kernel_end = .;
}