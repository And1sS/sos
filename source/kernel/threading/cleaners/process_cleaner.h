#ifndef SOS_PROCESS_CLEANER_H
#define SOS_PROCESS_CLEANER_H

#include "../process.h"

void process_cleaner_init();

void process_cleaner_mark(process* proc);

#endif // SOS_PROCESS_CLEANER_H
