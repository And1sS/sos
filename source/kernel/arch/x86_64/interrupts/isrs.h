#ifndef SOS_ISRS_H
#define SOS_ISRS_H

#include "../../../lib/types.h"

typedef struct cpu_context* exception_handler(u64 error_code,
                                              struct cpu_context* context);
typedef struct cpu_context* irq_handler(struct cpu_context* context);

void mount_irq_handler(u8 irq_num, irq_handler* handler);
void mount_exception_handler(u8 exception_num, exception_handler* handler);

#endif // SOS_ISRS_H