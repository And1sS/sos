#include "threading.h"
#include "../lib/id_generator.h"
#include "process.h"
#include "thread_cleaner.h"

static id_generator id_gen;

extern process kernel_process;

void threading_init() {
    id_generator_init(&id_gen);
    if (!process_init(&kernel_process, true))
        panic("Can't init kernel process");

    thread_cleaner_init();
}

bool threading_allocate_tid(u64* result) {
    return id_generator_get_id(&id_gen, result);
}

bool threading_free_tid(u64 tid) { return id_generator_free_id(&id_gen, tid); }
