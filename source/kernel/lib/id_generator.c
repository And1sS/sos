#include "id_generator.h"

void id_generator_init(id_generator* generator) {
    generator->set = bitset_create();
    init_lock(&generator->lock);
}

u64 id_generator_get_id(id_generator* generator) {
    bool interrupts_enabled = spin_lock_irq_save(&generator->lock);
    u64 id = bitset_allocate_index(generator->set);
    spin_unlock_irq_restore(&generator->lock, interrupts_enabled);

    return id;
}

bool id_generator_free_id(id_generator* generator, u64 id) {
    bool interrupts_enabled = spin_lock_irq_save(&generator->lock);
    bool result = bitset_free_index(generator->set, id);
    spin_unlock_irq_restore(&generator->lock, interrupts_enabled);

    return result;
}