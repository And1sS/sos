#include "array_stack.h"

void init_stack(array_stack* stack, u64 size, u64* array) {
    stack->size = size;
    stack->entries = 0;
    stack->array = array;
}

bool push(array_stack* stack, u64 entry) {
    if (stack->entries + 1 > stack->size) {
        return false;
    }

    stack->array[stack->entries++] = entry;
    return true;
}

bool can_pop(array_stack* stack) { return stack->entries > 0; }

u64 pop(array_stack* stack) { return stack->array[--stack->entries]; }