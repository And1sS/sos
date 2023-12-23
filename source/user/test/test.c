void main() {
    while (1) {
        __asm__ volatile("movq $0xDEADBEEF, %%rdi\n"
                         "int $80"
                         :
                         :
                         : "memory");
    }
}