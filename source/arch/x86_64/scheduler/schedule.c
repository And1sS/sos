void schedule() {
    // TODO: add proper constant for soft irq, maybe rewrite to syscall
    __asm__ volatile("int $250" : : : "memory");
}
