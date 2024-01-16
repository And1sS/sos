ENTRY(_start)

KERNEL_OFFSET = 0xFFFF800000000000;
KERNEL_START = 0x100000;

SECTIONS
{
    . = KERNEL_START;

    kernel_start = .;

    /* Multiboot headers */
    .multiboot_data : { *(.multiboot_data) }

    /* Early 32 bit boot code for long mode and paging enablement should live in first 2MiB of Ram */
    .boot32_text : { *(boot32_text) }

    .boot32_data : { *(boot32_data) }

    .boot32_rodata : { *(boot32_rodata) }

    . += KERNEL_OFFSET;

    /* Early 64 bit boot code and data should live in first GiB of ram */
    .boot64_text ALIGN (4K) : AT (ADDR (.boot64_text) - KERNEL_OFFSET) { *(.boot64_text) }

    .boot64_data ALIGN (4K) : AT (ADDR (.boot64_data) - KERNEL_OFFSET) { *(.boot64_data) }

    .boot64_rodata ALIGN (4K) : AT (ADDR (.boot64_rodata) - KERNEL_OFFSET) { *(.boot64_rodata) }


    /* Kernel itself */
    .text ALIGN (4K) : AT (ADDR (.text) - KERNEL_OFFSET) { *(.text) }

    .rodata ALIGN (4K) : AT (ADDR (.rodata) - KERNEL_OFFSET) { *(.rodata) }

    .data ALIGN (4K) : AT (ADDR (.data) - KERNEL_OFFSET) { *(.data) }

    .bss ALIGN (4K) : AT (ADDR (.bss) - KERNEL_OFFSET) { *(.bss) }

    kernel_end = .;
}
