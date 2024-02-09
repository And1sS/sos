#include "../lib/kprint.h"
#include "../lib/util.h"

struct cpu_context;

u64 sys_print(u64 arg0, struct cpu_context* context) {
    UNUSED(context);
    print((string) arg0);

    return 0;
}

u64 sys_print_u64(u64 arg0, struct cpu_context* context) {
    UNUSED(context);
    print_u64(arg0);

    return 0;
}