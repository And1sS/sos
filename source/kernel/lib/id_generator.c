#include "id_generator.h"

bool id_generator_init(id_generator* generator) {
    generator->set = bitset_create();
    generator->lock = SPIN_LOCK_STATIC_INITIALIZER;

    return generator->set != NULL;
}

void id_generator_deinit(id_generator* generator) {
    bitset_destroy(generator->set);
}

bool id_generator_get_id(id_generator* generator, u64* result) {
    bool interrupts_enabled = spin_lock_irq_save(&generator->lock);
    bool allocated = bitset_allocate_index(generator->set, result);
    spin_unlock_irq_restore(&generator->lock, interrupts_enabled);
    return allocated;
}

bool id_generator_free_id(id_generator* generator, u64 id) {
    bool interrupts_enabled = spin_lock_irq_save(&generator->lock);
    bool result = bitset_free_index(generator->set, id);
    spin_unlock_irq_restore(&generator->lock, interrupts_enabled);

    return result;
}