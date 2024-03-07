#include "tss.h"
#include "../../../lib/memory_util.h"
#include "../../../threading/scheduler.h"
#include "gdt.h"

task_state_segment tss;

void tss_init() {
    memset(&tss, 0, sizeof(task_state_segment));

    __asm__ volatile("ltr %0" : : "r"((u16) TSS_SEGMENT_SELECTOR));
}

void tss_update_rsp(u64 rsp) {
    tss.rsp0_low = rsp & 0xFFFFFFFF;
    tss.rsp0_high = (rsp >> 32) & 0xFFFFFFFF;
}

void update_tss() {
    thread* current = get_current_thread();
    if (current) {
        tss_update_rsp((u64) current->kernel_stack + THREAD_KERNEL_STACK_SIZE);
    }
}
