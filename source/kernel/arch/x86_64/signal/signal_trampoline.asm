bits 64

section .text

; Signal trampoline code, which will reside on user stack.
; When returning from signal handler, user will jump to this trampoline,
; which in turn will make system call to return from signal

global signal_trampoline_code_start
global signal_trampoline_code_end

signal_trampoline_code_start:
    mov rax, 4 ; SYS_SIGRET
    int 0x80
signal_trampoline_code_end:
