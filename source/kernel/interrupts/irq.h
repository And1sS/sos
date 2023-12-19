#ifndef SOS_INTERRUPTS_H
#define SOS_INTERRUPTS_H

#include "../lib/types.h"

bool local_irq_enabled();
void local_irq_enable();
void local_irq_disable();

bool local_irq_save();
void local_irq_restore(bool interrupts_enabled);

#endif // SOS_INTERRUPTS_H
