#ifndef SOS_INTERRUPTS_H
#define SOS_INTERRUPTS_H

#include "../types.h"

bool interrupts_enabled();
void enable_interrupts();
void disable_interrupts();

#endif // SOS_INTERRUPTS_H
