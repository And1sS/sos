#include "efer.h"
#include "msr.h"

#define EFER_MSR 0xC0000080

u64 efer_read() { return msr_read(EFER_MSR); }

void efer_write(u64 efer) { msr_write(EFER_MSR, efer); }