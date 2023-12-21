bits 64

section .text

start:
    mov rdi, 0x100E
    int 80
    jmp start
