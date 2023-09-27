#ifndef SOS_ARRAY_STACK_H
#define SOS_ARRAY_STACK_H

#include "../types.h"

typedef struct {
    u64 size;
    u64 entries;
    u64* array;
} array_stack;

void init_stack(array_stack* stack, u64 size, u64* array);
bool push(array_stack* stack, u64 entry);
bool can_pop(array_stack* stack);
u64 pop(array_stack* stack);

#endif // SOS_ARRAY_STACK_H
