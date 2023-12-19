int main() {
    __asm__ volatile("movq 0xDEADBEEF, %%rax\n"
                     "int $80"
                     :
                     :
                     : "rax");
    return 12;
}