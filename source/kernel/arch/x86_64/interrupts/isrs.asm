; Irq handlers entry-points.

; Hardware irqs are handled with interrupts disabled.

; Each handler pushes current cpu context on the stack (each
; general purpose register) and restores provided cpu context
; after calling C handler.

bits 64

section .text

; C handlers defined in isrs.c
extern handle_software_interrupt
extern handle_hardware_interrupt
extern handle_syscall


extern update_tss ; defined in tss.c
extern check_pending_signals ; defined in signal.c


%macro save_ds 0
    mov bx, ds
    push rbx
    mov rbx, 0x10
    mov ds, bx
%endmacro

%macro restore_ds 0
    pop rbx
    mov ds, bx
%endmacro

%macro save_state 0
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
    save_ds
%endmacro

%macro restore_state 0
    call update_tss
    call check_pending_signals

    restore_ds
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
%endmacro

%macro isr_soft_no_error_code 1
global isr_%1
isr_%1:
    save_state

    mov rdi, %1  ; interrupt number
    mov rsi, 0   ; error code
    mov rdx, rsp ; cpu_context pointer
    call handle_software_interrupt

    ; Now we run in probably new context
    mov rsp, rax

    restore_state
    iretq
%endmacro

%macro isr_soft_error_code 1
global isr_%1
isr_%1:
    pop rsi      ; error code
    save_state

    mov rdi, %1  ; interrupt number
    mov rdx, rsp ; cpu_context pointer
    call handle_software_interrupt

    ; Now we run in probably new context
    mov rsp, rax

    restore_state
    iretq
%endmacro

%macro isr_hard 1
global isr_%1
isr_%1:
    cli
    save_state

    mov rdi, %1
    mov rsi, rsp
    call handle_hardware_interrupt

    ; Now we run in probably new context
    mov rsp, rax

    restore_state
    iretq
%endmacro

isr_soft_no_error_code 0
isr_soft_no_error_code 1
isr_soft_no_error_code 2
isr_soft_no_error_code 3
isr_soft_no_error_code 4
isr_soft_no_error_code 5
isr_soft_no_error_code 6
isr_soft_no_error_code 7
isr_soft_error_code 8
isr_soft_no_error_code 9
isr_soft_error_code 10
isr_soft_error_code 11
isr_soft_error_code 12
isr_soft_error_code 13
isr_soft_error_code 14
isr_soft_no_error_code 15
isr_soft_no_error_code 16
isr_soft_error_code 17
isr_soft_no_error_code 18
isr_soft_no_error_code 19
isr_soft_no_error_code 20
isr_soft_error_code 21
isr_soft_no_error_code 22
isr_soft_no_error_code 23
isr_soft_no_error_code 24
isr_soft_no_error_code 25
isr_soft_no_error_code 26
isr_soft_no_error_code 27
isr_soft_no_error_code 28
isr_soft_error_code 29
isr_soft_error_code 30
isr_soft_no_error_code 31

isr_hard 32
isr_hard 33
isr_hard 34
isr_hard 35
isr_hard 36
isr_hard 37
isr_hard 38
isr_hard 39
isr_hard 40
isr_hard 41
isr_hard 42
isr_hard 43
isr_hard 44
isr_hard 45
isr_hard 46
isr_hard 47

; Syscall irq handler

; For now syscalls are implemented using obsolete "int 0x80",
; this is done to get them up and running quickly.
; This interface may change in favor of modern "syscall" interface later.
global isr_128
isr_128:
    save_state

    push rsp ; cpu context, 7th argument of C handler
    push rax ; syscall number, 6th argument of C handler
    call handle_syscall

    ; Now we run in probably new context
    mov rsp, rax

    restore_state
    iretq

; Schedule irq handler to call scheduler, this is dirty hack
; to get easy interface to call scheduler
isr_soft_no_error_code 250
