#include "threading.h"
#include "../lib/id_generator.h"
#include "thread_cleaner.h"

static id_generator tid_gen;

void threading_init() {
    if (!id_generator_init(&tid_gen))
        panic("Can't init tid generator");
}

bool threading_allocate_tid(u64* result) {
    return id_generator_get_id(&tid_gen, result);
}

bool threading_free_tid(u64 tid) { return id_generator_free_id(&tid_gen, tid); }
