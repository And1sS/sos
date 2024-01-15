bits 32

section .boot32_text

%define KERNEL_STACK_SIZE 8192

%define KERNEL_START_VADDR 0xFFFF800000000000
%define KERNEL_VMAPPED_RAM_START_VADDR 0XFFFF888000000000
%define KERNEL_START_ADDRESS 0x100000

%define V2P(addr) ((addr) - KERNEL_START_VADDR) ; virtual address to physical

%define CODE_SEGMENT_SELECTOR 1 << 3

KERNEL_STARTUP_MESSAGE_STR db 'Booting sos kernel in long mode...', 0
MULTIBOOT_ERROR_STR db 'Multiboot2 is not configured.', 0
CPUID_ERROR_STR db 'Cpuid instruction is not supported.', 0
LONG_MODE_ERROR_STR db 'Long mode is not supported.', 0

extern kernel_main
extern boot_print_string
extern boot_panic
extern boot_move_cursor_next_line
extern boot_clear_screen

global _start
_start:
    mov esp, V2P(kernel_stack + KERNEL_STACK_SIZE)

    push ebx ; to preserve multiboot structure address loaded by grub
    push eax ; to preserve eax register which should hold multiboot magic

    call boot_clear_screen
    mov esi, KERNEL_STARTUP_MESSAGE_STR
    call boot_print_string
    call boot_move_cursor_next_line

    pop eax
    call check_multiboot

    call check_cpuid
    call check_long_mode

    call set_up_page_tables
    call enable_paging

    lgdt [V2P(GDT64.pointer)]
    jmp CODE_SEGMENT_SELECTOR:V2P(long_mode_start)


%define MULTIBOOT2_MAGIC 0x36D76289
check_multiboot:
    cmp eax, MULTIBOOT2_MAGIC
    jne .no_multiboot
    ret
.no_multiboot:
    mov esi, MULTIBOOT_ERROR_STR
    call boot_panic



%define ID_ATTR 1 << 21

; Check if CPUID is supported by attempting to flip the ID bit (bit 21)
; in the FLAGS register. If we can flip it, CPUID is available.
check_cpuid:
    pushfd
    pop eax

    mov ecx, eax
    xor eax, ID_ATTR
    push eax
    popfd

    pushfd
    pop eax
    push ecx
    popfd

    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov esi, CPUID_ERROR_STR
    call boot_panic


%define CPUID_LONG_MODE_ATTR 1 << 29
%define CPUID_MAXIMUM_ROUTINE 0x80000000
%define CPUID_VERSION_INFORMATION_ROUTINE 0x80000001

check_long_mode:
    ; test if extended processor info in available
    mov eax, CPUID_MAXIMUM_ROUTINE                      ; implicit argument for cpuid
    cpuid
    cmp eax, CPUID_VERSION_INFORMATION_ROUTINE
    jb .no_long_mode

    mov eax, CPUID_VERSION_INFORMATION_ROUTINE
    cpuid                  
    test edx, CPUID_LONG_MODE_ATTR                      ; implicit return data
    jz .no_long_mode       
    ret
.no_long_mode:
    mov esi, LONG_MODE_ERROR_STR
    call boot_panic


%define PRESENT_ATTR 1 << 0
%define WRITABLE_ATTR 1 << 1
%define HUGE_PAGE_ATTR 1 << 7 ; creates a 1 GiB page in PML3, and 2 MiB page in PML2
%define PAGE_SIZE 4096
%define HUGE_PAGE_SIZE_2MB 0x200000
%define HUGE_PAGE_SIZE_1GB 0x40000000
%define PT_ENTRIES 512
%define PT_KERNEL_SPACE_START_IDX 256 ; 0xFFFF800000000000
%define PT_KERNEL_SPACE_VMAPPED_RAM_START_IDX 273 ; 0XFFFF888000000000

%define VMAPPED_RAM_P1_TABLE vmapped_ram_tables
%define VMAPPED_RAM_P2_TABLE vmapped_ram_tables + 4096
%define VMAPPED_RAM_P3_TABLE vmapped_ram_tables + 8192

; This routine:
; 1) sets up identity mapping of first 2MiB RAM to (0 - 0x200000) address space
; 2) sets up identity mapping of first 2MiB RAM to (0XFFFF888000000000 - 0XFFFF888000200000) address space
; 3) sets up identity mapping of first 1GiB RAM to kernel space (0xFFFF800000000000 - 0xFFFF800040000000)
set_up_page_tables:
    ; mapping 1 GiB of kernel address space
    mov ecx, PT_ENTRIES
    mov ebx, V2P(kernel_p2_table)
    mov eax, HUGE_PAGE_ATTR | PRESENT_ATTR | WRITABLE_ATTR
.identity_map_kernel:
    mov [ebx], eax
    add ebx, 8
    add eax, HUGE_PAGE_SIZE_2MB
    loop .identity_map_kernel

    mov eax, V2P(kernel_p2_table)
    or eax, PRESENT_ATTR | WRITABLE_ATTR
    mov [V2P(kernel_p3_table)], eax

    mov eax, V2P(kernel_p3_table)
    or eax, PRESENT_ATTR | WRITABLE_ATTR
    mov [V2P(kernel_p4_table) + PT_KERNEL_SPACE_START_IDX * 8], eax

    ; identity mapping of 2 MiB of ram to (0 - 0x200000)
    ; and (0XFFFF888000000000 - 0XFFFF888000200000) address spaces

    ; (0 - 2MiB) is needed to not triple fault after enabling paging
    ; while code still runs in low memory address space and will be
    ; unmapped before jumping to kernel

    ; (0XFFFF888000000000 - 0XFFFF888000200000) is mapped
    ; to run early 64 bit code
    mov ecx, PT_ENTRIES
    mov ebx, V2P(VMAPPED_RAM_P1_TABLE)
    mov eax, PRESENT_ATTR | WRITABLE_ATTR
.identity_map_lowmem_ram:
    mov [ebx], eax
    add ebx, 8
    add eax, PAGE_SIZE
    loop .identity_map_lowmem_ram

    mov eax, V2P(VMAPPED_RAM_P1_TABLE)
    or eax, PRESENT_ATTR | WRITABLE_ATTR
    mov [V2P(VMAPPED_RAM_P2_TABLE)], eax

    mov eax, V2P(VMAPPED_RAM_P2_TABLE)
    or eax, PRESENT_ATTR | WRITABLE_ATTR
    mov [V2P(VMAPPED_RAM_P3_TABLE)], eax

    mov eax, V2P(VMAPPED_RAM_P3_TABLE)
    or eax, PRESENT_ATTR | WRITABLE_ATTR
    mov [V2P(kernel_p4_table)], eax
    mov [V2P(kernel_p4_table) + PT_KERNEL_SPACE_VMAPPED_RAM_START_IDX * 8], eax

    ret


%define PAE_ATTR 1 << 5
%define EFER_LONG_MODE_ATTR 1 << 8
%define PAGING_ENABLED_ATTR 1 << 31

enable_paging:
    mov eax, V2P(kernel_p4_table)
    mov cr3, eax

    mov eax, cr4
    or eax, PAE_ATTR
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, EFER_LONG_MODE_ATTR
    wrmsr

    mov eax, cr0
    or eax, PAGING_ENABLED_ATTR
    mov cr0, eax

    ret

section .boot64_data
align 4096
kernel_stack:
    resb KERNEL_STACK_SIZE

global kernel_p4_table
; kernel page tables
align 4096
kernel_p4_table: ; root pml4
    times PT_ENTRIES dq 0

; page tables for kernel identity mapping of 1GiB or kernel
; address space
align 4096
kernel_p3_table:
    times PT_ENTRIES dq 0
align 4096
kernel_p2_table:
    times PT_ENTRIES dq 0
align 4096
kernel_p1_table:
    times PT_ENTRIES dq 0

; 128 page tables, used to identity map 64TB of physical ram
align 4096
vmapped_ram_tables:
    times 128 * PT_ENTRIES dq 0


bits 64

section .boot64_rodata

%define CODE_ATTR 1 << 43
%define CONFORMING_ATTR 1 << 44
%define SEGMENT_PRESENT_ATTR 1 << 47
%define LONG_MODE_ATTR 1 << 53


; temporary gdt to get to long mode and resetup it later properly
GDT64:
    dq 0 
.code:
    dq CODE_ATTR | CONFORMING_ATTR | SEGMENT_PRESENT_ATTR | LONG_MODE_ATTR
.pointer:
    dw $ - GDT64 - 1
    dq GDT64

section .boot64_text

extern kernel_main
extern set_up_64tb_ram_identity_mapping

long_mode_start:
    ; now rsp contains physical address, we need to adjust it to with virtual
    mov rax, KERNEL_START_VADDR
    add rsp, rax

    mov rax, qword .upper_memory
    jmp rax

.upper_memory:
    ; Remove identity mapping of first two megs of ram used to run lower memory code
    mov qword rax, 0
    mov [qword kernel_p4_table], rax
    call reload_cr3

    call remap_kernel
    ; now we can call any C kernel function

    mov rdi, kernel_p4_table
    mov rsi, vmapped_ram_tables
    call set_up_64tb_ram_identity_mapping
    call reload_cr3

    ; here stack contains argument of multiboot structure, which should be passed wia rdi register
    ; following System V AMD64 ABI calling convention
    pop rdi
    call kernel_main

    jmp $
    ret ; unreachable


; In low memory only 1GiB of kernel space were mapped, this can be expanded so that
; kernel of any size will have fully mapped code.
; So, this routine will map 512GiB address space (0xFFFF800000000000 - 0xFFFF87FFFFFFFFFF) for kernel code,
; by using huge pages on PML3 level rather then PML2.
remap_kernel:
    mov rcx, PT_ENTRIES
    mov rbx, kernel_p3_table
    mov rax, HUGE_PAGE_ATTR | PRESENT_ATTR | WRITABLE_ATTR
.identity_map_kernel:
    mov [rbx], rax
    add rbx, 8
    add rax, HUGE_PAGE_SIZE_1GB
    loop .identity_map_kernel

    call reload_cr3
    ret


reload_cr3:
   mov rax, cr3
   mov cr3, rax

   ret