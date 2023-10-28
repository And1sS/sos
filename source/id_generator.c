#include "id_generator.h"

void init_id_generator(id_generator* generator) {
    generator->id_seq = 0;
    init_lock(&generator->lock);
}

u64 get_id(id_generator* generator) {
    bool interrupts_enabled = spin_lock_irq_save(&generator->lock);
    u64 id = generator->id_seq++;
    spin_unlock_irq_restore(&generator->lock, interrupts_enabled);

    return id;
}