#include "gdt.h"

typedef struct __attribute__((__aligned__(8), __packed__)) {
    u16 segment_limit_0_15;
    u32 base_addr_0_23 : 24;
    u8 flags_8_15;
    u8 flags_16_23;
    u8 base_addr_24_31;
} segment_descriptor;

typedef struct __attribute__((__packed__)) {
    const u16 limit;
    const segment_descriptor* data;
} gdt_descriptor;

const u8 SEGMENT_PRESENT_OFFSET = 7;
const u8 DESCRIPTOR_PRIVILEGE_LEVEL_OFFSET = 5;
const u8 DESCRIPTOR_TYPE_OFFSET = 4;
const u8 DATA_OR_CODE_OFFSET = 3;
const u8 EXPANSION_OFFSET = 2;
const u8 CONFORMING_OFFSET = 2;
const u8 WRITE_ENABLE_OFFSET = 1;
const u8 READ_ENABLE_OFFSET = 1;
const u8 BUSY_OFFSET = 1;
const u8 ACCESSED_OFFSET = 0;

segment_descriptor gdt_data[6];
const gdt_descriptor gdt = {.data = gdt_data, .limit = sizeof(gdt_data) - 1};

u8 gen_code_segment_descriptor_8_15_flags(bit present, u8 dpl, bit conforming,
                                          bit read_enabled, bit accessed) {

    return ((present & 1) << SEGMENT_PRESENT_OFFSET)
           | ((dpl & 0b11) << DESCRIPTOR_PRIVILEGE_LEVEL_OFFSET)
           | (1 << DESCRIPTOR_TYPE_OFFSET) | (1 << DATA_OR_CODE_OFFSET)
           | ((conforming & 1) << CONFORMING_OFFSET)
           | ((read_enabled & 1) << READ_ENABLE_OFFSET)
           | ((accessed & 1) << ACCESSED_OFFSET);
}

u8 gen_data_segment_descriptor_8_15_flags(bit present, u8 dpl,
                                          bit expansion_direction,
                                          bit write_enabled, bit accessed) {

    return ((present & 1) << SEGMENT_PRESENT_OFFSET)
           | ((dpl & 0b11) << DESCRIPTOR_PRIVILEGE_LEVEL_OFFSET)
           | (1 << DESCRIPTOR_TYPE_OFFSET) | (0 << DATA_OR_CODE_OFFSET)
           | ((expansion_direction & 1) << EXPANSION_OFFSET)
           | ((write_enabled & 1) << WRITE_ENABLE_OFFSET)
           | ((accessed & 1) << ACCESSED_OFFSET);
}

u8 gen_task_state_segment_descriptor_8_15_flags(bit present, u8 dpl, bit busy) {

    return ((present & 1) << SEGMENT_PRESENT_OFFSET)
           | ((dpl & 0b11) << DESCRIPTOR_PRIVILEGE_LEVEL_OFFSET)
           | (0 << DESCRIPTOR_TYPE_OFFSET) | ((busy & 1) << BUSY_OFFSET)
           | 0b1001;
}

const u8 GRANULARITY_OFFSET = 7;
const u8 DEFAULT_OPERATION_SIZE_OFFSET = 6;
const u8 LONG_MODE_SEGMENT_OFFSET = 5;
const u8 AVAILABLE_FOR_SYSTEM_OFFSET = 4;
const u8 SEGMENT_LIMIT_16_19_OFFSET = 0;

u8 gen_segment_descriptor_16_23_flags(bit granularity,
                                      bit default_operation_size, bit long_mode,
                                      bit available_for_system,
                                      u8 segment_limit_16_19) {

    return ((granularity & 1) << GRANULARITY_OFFSET)
           | ((default_operation_size & 1) << DEFAULT_OPERATION_SIZE_OFFSET)
           | ((long_mode & 1) << LONG_MODE_SEGMENT_OFFSET)
           | ((available_for_system & 1) << AVAILABLE_FOR_SYSTEM_OFFSET)
           | (segment_limit_16_19 << SEGMENT_LIMIT_16_19_OFFSET);
}

segment_descriptor gen_null_segment_decriptor(void) {
    segment_descriptor result = {};
    return result;
}

segment_descriptor gen_code_segment_descriptor(
    u32 base_addr, u32 segment_limit, bit granularity,
    bit default_operation_size, bit long_mode, bit available_for_system,
    bit present, bit dpl, bit conforming, bit read_enabled, bit accessed) {

    segment_descriptor result = {
        .segment_limit_0_15 = segment_limit & 0xFFFF,
        .base_addr_0_23 = base_addr & 0xFFFFFF,
        .flags_8_15 = gen_code_segment_descriptor_8_15_flags(
            present, dpl, conforming, read_enabled, accessed),
        .flags_16_23 = gen_segment_descriptor_16_23_flags(
            granularity, default_operation_size, long_mode,
            available_for_system, (segment_limit >> 16) & 0xF),
        .base_addr_24_31 = (base_addr >> 24) & 0xFF};

    return result;
}

segment_descriptor gen_data_segment_descriptor(
    u32 base_addr, u32 segment_limit, bit granularity,
    bit default_operation_size, bit long_mode, bit available_for_system,
    bit present, bit dpl, bit expansion_direction, bit write_enabled,
    bit accessed) {

    segment_descriptor result = {
        .segment_limit_0_15 = segment_limit & 0xFFFF,
        .base_addr_0_23 = base_addr & 0xFFFFFF,
        .flags_8_15 = gen_data_segment_descriptor_8_15_flags(
            present, dpl, expansion_direction, write_enabled, accessed),
        .flags_16_23 = gen_segment_descriptor_16_23_flags(
            granularity, default_operation_size, long_mode,
            available_for_system, (segment_limit >> 16) & 0xF),
        .base_addr_24_31 = (base_addr >> 24) & 0xFF};

    return result;
}

segment_descriptor gen_task_state_segment_descriptor(
    u32 base_addr, u32 segment_limit, bit granularity, bit available_for_system,
    bit present, bit dpl, bit busy) {

    segment_descriptor result = {
        .segment_limit_0_15 = segment_limit & 0xFFFF,
        .base_addr_0_23 = base_addr & 0xFFFFFF,
        .flags_8_15 =
            gen_task_state_segment_descriptor_8_15_flags(present, dpl, busy),
        .flags_16_23 = gen_segment_descriptor_16_23_flags(
            granularity, 0, 0, available_for_system,
            (segment_limit >> 16) & 0xF),
        .base_addr_24_31 = (base_addr >> 24) & 0xFF};

    return result;
}

u16 KERNEL_CODE_SEGMENT_SELECTOR = 1 << 3;
u16 KERNEL_DATA_SEGMENT_SELECTOR = 2 << 3;
u16 USER_CODE_SEGMENT_SELECTOR = 3 << 3;
u16 USER_DATA_SEGMENT_SELECTOR = 4 << 4;

void gdt_init(void) {
    gdt_data[0] = gen_null_segment_decriptor();

    // in long mode default operation size flag should be zero, base and limits
    // are ignored
    gdt_data[1] =
        gen_code_segment_descriptor(0, 0xFFFFF, 1, 0, 1, 1, 1, 0, 0, 0, 0);
    gdt_data[2] =
        gen_data_segment_descriptor(0, 0xFFFFF, 1, 0, 1, 1, 1, 0, 0, 1, 0);
    gdt_data[3] =
        gen_code_segment_descriptor(0, 0xFFFFF, 1, 0, 1, 0, 1, 3, 0, 0, 0);
    gdt_data[4] =
        gen_data_segment_descriptor(0, 0xFFFFF, 1, 0, 1, 0, 1, 3, 0, 1, 0);
    // gdt_data[5] = gen_task_state_segment_descriptor(0, 0xFFFFF, 1, 1, 1, 0,
    // 0);

    __asm__ volatile("    lgdt %0\n"
                     "    pushq %1\n"
                     "    movabs $tmp%=, %%rax\n"
                     "    pushq %%rax\n"
                     "    retfq\n" // far ret to force cs register reloading
                     "tmp%=:\n"
                     "    mov %2, %%ds\n"
                     "    mov %2, %%ss\n"
                     "    mov $0, %%rax\n"
                     "    mov %%rax, %%es\n"
                     "    mov %%rax, %%fs\n"
                     "    mov %%rax, %%gs"
                     :
                     : "m"(gdt), "r"((u64) KERNEL_CODE_SEGMENT_SELECTOR),
                       "r"(KERNEL_DATA_SEGMENT_SELECTOR));
}
