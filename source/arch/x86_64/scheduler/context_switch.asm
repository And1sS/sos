bits 64

section .text

extern scheduler_lock
extern spin_unlock

global _context_switch
global _resume_thread

; Calling convention - System V ABI
; arguments:
;   rdi - pointer to rsp of current thread to be saved
;   rsi - pointer to rsp of next running thread to be restored
_context_switch:
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

    mov [rdi], rsp

    ; now release the lock before jumping to next thread
    push rsi
    mov rdi, scheduler_lock
    call spin_unlock
    pop rsi

    mov rsp, [rsi]

_resume_thread:
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

    sti
    ret