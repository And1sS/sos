void schedule() {
    // TODO: add proper constant for soft irq
    __asm__ volatile("int $250" : : : "memory");
}
