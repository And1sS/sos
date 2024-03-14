#ifndef SOS_WAIT_H
#define SOS_WAIT_H

#include "../threading/thread.h"

#define WAIT_FOR_IRQ(lock, flags, cond)                                        \
    do {                                                                       \
        while (!(cond)) {                                                      \
            get_current_thread()->state = BLOCKED;                             \
            spin_unlock_irq_restore((lock), (flags));                          \
                                                                               \
            schedule();                                                        \
                                                                               \
            (flags) = spin_lock_irq_save((lock));                              \
        }                                                                      \
    } while (0)

#define WAIT_FOR_IRQ_INTERRUPTABLE(lock, flags, cond)                          \
    ({                                                                         \
        bool ___interrupted = false;                                           \
        bool ___condition_met = (cond);                                        \
                                                                               \
        while (!___condition_met && !___interrupted) {                         \
            get_current_thread()->state = BLOCKED;                             \
            spin_unlock_irq_restore((lock), (flags));                          \
                                                                               \
            schedule();                                                        \
                                                                               \
            (flags) = spin_lock_irq_save((lock));                              \
            ___condition_met = (cond);                                         \
            ___interrupted = thread_any_pending_signals();                     \
        }                                                                      \
                                                                               \
        !___condition_met&& ___interrupted;                                    \
    })

#endif // SOS_WAIT_H