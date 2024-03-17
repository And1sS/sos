; Irq handlers entry-points.

; Hardware irqs are handled with interrupts disabled.

; Each handler pushes current cpu context on the stack (each
; general purpose register) and restores provided cpu context
; after calling C handler.

bits 64

section .text

; C handlers defined in isrs.c
extern handle_exception
extern handle_soft_irq
extern handle_hard_irq

extern update_tss ; defined in tss.c
extern handle_pending_signals ; defined in signal.c


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

    mov rdi, rsp
    call handle_pending_signals

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

    add rsp, 8 ; remove error code from stack
%endmacro

; x86-64 exception with error code handler stub
%macro esr_error_code 1
global esr_%1
esr_%1:
    save_state

    mov rdi, %1  ; interrupt number
    mov rsi, [rsp + 16 * 8] ; stack top initially holds error code,
                            ; we move it here so that we don't scratch
                            ; rsi value before saving state

    mov rdx, rsp ; cpu_context pointer
    call handle_exception

    ; Now we run in probably new context
    mov rsp, rax

    restore_state

    iretq
%endmacro


; x86-64 exception without error code handler stub
%macro esr_no_error_code 1
global esr_%1
esr_%1:
    push 0 ; push fake error code
    save_state

    mov rdi, %1  ; interrupt number
    mov rsi, 0   ; error code
    mov rdx, rsp ; cpu_context pointer
    call handle_exception

    ; Now we run in probably new context
    mov rsp, rax

    restore_state
    iretq
%endmacro


; Software interrupts handler stub
%macro isr_soft 1
global isr_%1
isr_%1:
    push 0 ; push fake error code
    save_state

    mov rdi, %1  ; interrupt number
    mov rsi, rsp ; cpu_context pointer
    call handle_soft_irq

    ; Now we run in probably new context
    mov rsp, rax

    restore_state
    iretq
%endmacro


; Hardware interrupts handler stub
%macro isr_hard 1
global isr_%1
isr_%1:
    cli
    push 0 ; push fake error code
    save_state

    mov rdi, %1
    mov rsi, rsp
    call handle_hard_irq

    ; Now we run in probably new context
    mov rsp, rax

    restore_state
    iretq
%endmacro


esr_no_error_code 0
esr_no_error_code 1
esr_no_error_code 2
esr_no_error_code 3
esr_no_error_code 4
esr_no_error_code 5
esr_no_error_code 6
esr_no_error_code 7
esr_error_code 8
esr_no_error_code 9
esr_error_code 10
esr_error_code 11
esr_error_code 12
esr_error_code 13
esr_error_code 14
esr_no_error_code 15
esr_no_error_code 16
esr_error_code 17
esr_no_error_code 18
esr_no_error_code 19
esr_no_error_code 20
esr_error_code 21
esr_no_error_code 22
esr_no_error_code 23
esr_no_error_code 24
esr_no_error_code 25
esr_no_error_code 26
esr_no_error_code 27
esr_no_error_code 28
esr_error_code 29
esr_error_code 30
esr_no_error_code 31

%assign i 32
%rep   48 - 32
       isr_hard i
%assign i i+1
%endrep

%assign i 48
%rep    250 - 48
        isr_soft i
%assign i i+1
%endrep

; Scheduler specific stubs
extern scheduler_lock
extern scheduler_unlock
extern context_switch
extern get_current_thread
extern thread_unlock

; isr #250 stub, schedule() implementation
; Exists only because context switch should be done atomically
global isr_250
isr_250:
    cli
    push 0 ; push fake error code
    save_state

    call scheduler_lock

    mov rdi, rsp
    call context_switch

    ; Now we run in probably new context
    mov rsp, rax

    call scheduler_unlock

    restore_state
    iretq


; isr #251 stub, schedule_thread_exit() implementation
; Exists only because thread unlock should be performed in new context after atomic context switch
global isr_251
isr_251:
    cli
    push 0 ; push fake error code
    save_state

    call get_current_thread
    push rax ; push dead thread pointer

    call scheduler_lock

    mov rdi, rsp
    call context_switch

    pop rdi ; restore previous thread pointer

    ; Now we run in probably new context
    mov rsp, rax

    push rdi ; save dead thread again
    call scheduler_unlock

    pop rdi ; restore in again
    call thread_unlock ; unlock dead thread lock, so thread cleaner can do its job

    restore_state
    iretq


%assign i 252
%rep    256 - 252
        isr_soft i
%assign i i+1
%endrep