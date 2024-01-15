bits 32

%define KERNEL_START_VADDR 0xFFFF800000000000
%define V2P(addr) ((addr) - KERNEL_START_VADDR) ; virtual address to physical

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

section .boot32_text

; This routine:
; 1) sets up identity mapping of first 2MiB RAM to (0 - 0x200000) address space
; 2) sets up identity mapping of first 2MiB RAM to (0XFFFF888000000000 - 0XFFFF888000200000) address space
; 3) sets up identity mapping of first 1GiB RAM to kernel space (0xFFFF800000000000 - 0xFFFF800040000000)
global set_up_page_tables
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

global enable_paging
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

bits 64

section .boot64_text

; Remove identity mapping of first two megs of ram used to run lower memory code
global remove_low_mem_identity_mapping
remove_low_mem_identity_mapping:
    mov qword rax, 0
    mov [qword kernel_p4_table], rax
    call reload_cr3
    ret


; In low memory only 1GiB of kernel space were mapped, this can be expanded so that
; kernel of any size will have fully mapped code.
; So, this routine will map 512GiB address space (0xFFFF800000000000 - 0xFFFF87FFFFFFFFFF) for kernel code,
; by using huge pages on PML3 level rather then PML2.
global remap_kernel
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


global reload_cr3
reload_cr3:
    mov rax, cr3
    mov cr3, rax

    ret


extern set_up_64tb_ram_identity_mapping
global remap_ram
remap_ram:
    mov rdi, kernel_p4_table
    mov rsi, vmapped_ram_tables
    call set_up_64tb_ram_identity_mapping
    call reload_cr3
    ret


section .boot64_data

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
