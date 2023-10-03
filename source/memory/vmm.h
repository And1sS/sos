#ifndef SOS_VIRTUAL_MEMORY_MANAGER_H
#define SOS_VIRTUAL_MEMORY_MANAGER_H

#include "pmm.h"

#define P1_OFFSET(a) (((a) >> 12) & 0x1FF)
#define P2_OFFSET(a) (((a) >> 21) & 0x1FF)
#define P3_OFFSET(a) (((a) >> 30) & 0x1FF)
#define P4_OFFSET(a) (((a) >> 39) & 0x1FF)

#endif // SOS_VIRTUAL_MEMORY_MANAGER_H
