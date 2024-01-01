#include "cpu_context.h"
#include "../../../lib/memory_util.h"
#include "../../../memory/heap/kheap.h"
#include "../../common/context.h"
#include "gdt.h"

bool arch_is_userspace_context(struct cpu_context* context) {
    cpu_context* arch_context = (cpu_context*) context;
    return arch_context->cs == USER_CODE_SEGMENT_SELECTOR;
}
