#include "threading.h"
#include "../lib/id_generator.h"
#include "thread_cleaner.h"

static id_generator tid_gen;
static id_generator pid_gen;

extern process kernel_process;

void threading_init() {
    if (!id_generator_init(&tid_gen))
        panic("Can't init tid generator");
    if (!id_generator_init(&pid_gen))
        panic("Can't init pid generator");

    if (!process_init(NULL, &kernel_process, true))
        panic("Can't init kernel process");

    thread_cleaner_init();
}

bool threading_allocate_tid(u64* result) {
    return id_generator_get_id(&tid_gen, result);
}

bool threading_free_tid(u64 tid) { return id_generator_free_id(&tid_gen, tid); }

bool threading_allocate_pid(u64* result) {
    return id_generator_get_id(&pid_gen, result);
}

bool threading_free_pid(u64 pid) { return id_generator_free_id(&pid_gen, pid); }
