ENTRY(_start)

KERNEL_OFFSET = 0xFFFF800000000000;
KERNEL_START = 0x100000;

SECTIONS
{
    . = KERNEL_START;

    kernel_start = .;

    /* Multiboot headers */
    .multiboot.data : { *(.multiboot.data) }

    /* Early 32 bit boot code for long mode and paging enablement should live in first 2MiB of Ram */
    .boot32.text : { *(.multiboot.text) }


    . += KERNEL_OFFSET;

    /* Early 64 bit boot code and data should live in first GiB of ram */
    .boot64.text ALIGN (4K) : AT (ADDR (.boot64.text) - KERNEL_OFFSET) { *(.boot64.text) }

    .boot64.data ALIGN (4K) : AT (ADDR (.boot64.data) - KERNEL_OFFSET) { *(.boot64.data) }

    .boot64.rodata ALIGN (4K) : AT (ADDR (.boot64.rodata) - KERNEL_OFFSET) { *(.boot64.rodata) }


    /* Kernel itself */
    .text ALIGN (4K) : AT (ADDR (.text) - KERNEL_OFFSET) { *(.text) }

    .rodata ALIGN (4K) : AT (ADDR (.rodata) - KERNEL_OFFSET) { *(.rodata) }

    .data ALIGN (4K) : AT (ADDR (.data) - KERNEL_OFFSET) { *(.data) }

    .bss ALIGN (4K) : AT (ADDR (.bss) - KERNEL_OFFSET) { *(.bss) }

    kernel_end = .;
}
