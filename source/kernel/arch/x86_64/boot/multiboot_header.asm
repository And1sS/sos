bits 32

%define MAGIC_NUMBER 0xE85250D6
%define ARCHITECTURE 0 ; i386 protected mode
%define HEADER_LENGTH 16
%define CHECKSUM -MAGIC_NUMBER - ARCHITECTURE - HEADER_LENGTH

section .multiboot_data
; Multiboot2 header

align 8

dd MAGIC_NUMBER
dd ARCHITECTURE
dd HEADER_LENGTH
dd CHECKSUM

; empty tag

dw 0
dw 0
dd 8

; Multiboot2 header end