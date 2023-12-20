void main() {
    for (int i = 0; i < 12; ++i) {
        __asm__ volatile("movq 0xDEADBEEF, %%rax\n"
                         "int $80"
                         :
                         :
                         : "rax");
    }

    __asm__ volatile("movq $12, %%rdi\n"
                     "int $80"
                     :
                     :
                     : "rdi");
}