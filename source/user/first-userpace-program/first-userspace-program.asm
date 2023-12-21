bits 64

section .text

start:
    call trigger_temp_syscall
    jmp start

trigger_page_fault_missing:
    mov rax, US_UNMAPPED
    mov rbx, [rax]
    ret

trigger_page_fault_protection_violation:
    mov rax, KS_MAPPED
    mov rbx, [rax]
    ret

trigger_protection_violation:
   ltr [US_MAPPED]
   ret

trigger_temp_syscall:
    mov rax, 0

.loop:
    inc rax
    cmp rax, 10000000
    jne .loop

    mov rdi, 0x100E
    int 80
    ret

US_MAPPED equ 0x1300
US_UNMAPPED equ 0x7cccfffffff
KS_MAPPED equ 0xffffc88000000000
