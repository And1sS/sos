#ifndef SOS_EFER_H
#define SOS_EFER_H

#include "../../../lib/types.h"

#define EFER_SYSCALL_ENABLE 0x00000001
#define EFER_NX_ENABLE 0x00000800

u64 efer_read();

void efer_write(u64 efer);

#endif // SOS_EFER_H
