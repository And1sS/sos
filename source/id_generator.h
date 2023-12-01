#ifndef SOS_ID_GENERATOR_H
#define SOS_ID_GENERATOR_H

#include "lib/bitset.h"
#include "spin_lock.h"

typedef struct {
    bitset* set;
    lock lock;
} id_generator;

void id_generator_init(id_generator* generator);
u64 id_generator_get_id(id_generator* generator);
bool id_generator_free_id(id_generator* generator, u64 id);

#endif
