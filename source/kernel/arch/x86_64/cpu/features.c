#include "features.h"
#include "cpuid.h"
#include "efer.h"

static bool execute_disable_supported;

void features_init() {
    u32 eax;
    u32 ebx;
    u32 ecx;
    u32 edx;
    cpuid(CPUID_EXT_FEATURES, &eax, &ebx, &ecx, &edx);

    execute_disable_supported =
        (edx >> CPUID_EXECUTE_DISABLE_FEATURE_OFFSET) & 1;

    if (execute_disable_supported)
        efer_write(efer_read() | EFER_NX_ENABLE);
}

bool features_execute_disable_supported() { return execute_disable_supported; }