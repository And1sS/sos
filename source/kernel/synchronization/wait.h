#ifndef SOS_WAIT_H
#define SOS_WAIT_H

#include "../threading/process.h"
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
        bool ___interrupted =                                                  \
            thread_any_pending_signals() || process_any_pending_signals();     \
        bool ___condition_met = (cond);                                        \
                                                                               \
        while (!___condition_met && !___interrupted) {                         \
            get_current_thread()->state = BLOCKED;                             \
                                                                               \
            /* need to recheck here, because someone could send a signal after \
             * interrupted check, but before setting state to BLOCKED, which   \
             * wouldn't add current thread to scheduler run queue (since       \
             * thread is currently running), but would set thread state to     \
             * RUNNING, but since we have overridden it here - we have to      \
             * recheck that this didn't happen                                 \
             */                                                                \
            ___interrupted =                                                   \
                thread_any_pending_signals() || process_any_pending_signals(); \
            if (___interrupted) {                                              \
                /* restore state back to what it was before */                 \
                get_current_thread()->state = RUNNING;                         \
                break;                                                         \
            }                                                                  \
                                                                               \
            spin_unlock_irq_restore((lock), (flags));                          \
                                                                               \
            schedule();                                                        \
                                                                               \
            (flags) = spin_lock_irq_save((lock));                              \
            ___condition_met = (cond);                                         \
            ___interrupted =                                                   \
                thread_any_pending_signals() || process_any_pending_signals(); \
        }                                                                      \
                                                                               \
        !___condition_met&& ___interrupted;                                    \
    })

#endif // SOS_WAIT_H