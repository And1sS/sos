#ifndef SOS_ID_GENERATOR_H
#define SOS_ID_GENERATOR_H

#include "spin_lock.h"

typedef struct {
    u64 id_seq;
    lock lock;
} id_generator;

void init_id_generator(id_generator* generator);
u64 get_id(id_generator* generator);

#endif
