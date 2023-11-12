bits 64

section .text

extern handle_software_interrupt
extern handle_hardware_interrupt

%macro push_regs 0
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
%endmacro

%macro pop_regs 0
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

%macro isr_soft 1
global isr_%1
isr_%1:
    cli
    pop rsi
    push_regs
    mov rdi, %1
    mov rdx, rsp
    call handle_software_interrupt
    mov rsp, rax
    pop_regs
    iretq
%endmacro

%macro isr_hard 1
global isr_%1
isr_%1:
    cli
    push_regs
    mov rdi, %1
    mov rsi, rsp
    call handle_hardware_interrupt
    mov rsp, rax
    pop_regs
    iretq
%endmacro

isr_soft 0
isr_soft 1
isr_soft 2
isr_soft 3
isr_soft 4
isr_soft 5
isr_soft 6
isr_soft 7
isr_soft 8
isr_soft 9
isr_soft 10
isr_soft 11
isr_soft 12
isr_soft 13
isr_soft 14
isr_soft 15
isr_soft 16
isr_soft 17
isr_soft 18
isr_soft 19
isr_soft 20
isr_soft 21
isr_soft 22
isr_soft 23
isr_soft 24
isr_soft 25
isr_soft 26
isr_soft 27
isr_soft 28
isr_soft 29
isr_soft 30
isr_soft 31

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
