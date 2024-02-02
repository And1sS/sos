bits 64

org 0x1000

section .text

%macro make_syscall 1
    mov rax, %1
    int 0x80
%endmacro

start:
    call trigger_temp_syscall
    jmp start

;trigger_page_fault_missing:
;    mov rax, US_UNMAPPED
;    mov rbx, [rax]
;    ret
;
;trigger_page_fault_protection_violation:
;    mov rax, KS_MAPPED
;    mov rbx, [rax]
;    ret
;
;trigger_protection_violation:
;   ltr [US_MAPPED]
;   ret

trigger_temp_syscall:
    mov rbx, 0

.loop:
    inc rbx
    cmp rbx, 10000000 ; loop a bit to not flood with syscalls
    jne .loop

    mov rdi, HELLO_WORLD_STR ; syscall arg0
    make_syscall 0
    ret

HELLO_WORLD_STR: db 'Hello world from user space! VMM revision', 0

US_MAPPED equ 0x1300
US_UNMAPPED equ 0x7cccfffffff
KS_MAPPED equ 0xffffc88000000000
