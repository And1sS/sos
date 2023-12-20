ENTRY(_start)

KERNEL_OFFSET = 0xFFFF800000000000;
KERNEL_START = 0x100000;

SECTIONS
{
    . = KERNEL_START;

    kernel_start = .;

    .multiboot.data : { *(.multiboot.data) }

    .multiboot.text : { *(.multiboot.text) }

    . += KERNEL_OFFSET;

    .text ALIGN (4K) : AT (ADDR (.text) - KERNEL_OFFSET) { *(.text) }

    .rodata ALIGN (4K) : AT (ADDR (.rodata) - KERNEL_OFFSET) { *(.rodata) }

    .data ALIGN (4K) : AT (ADDR (.data) - KERNEL_OFFSET) { *(.data) }

    .bss ALIGN (4K) : AT (ADDR (.bss) - KERNEL_OFFSET) { *(.bss) }

    kernel_end = .;
}
