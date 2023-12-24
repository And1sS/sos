#ifndef SOS_SIGNAL_H
#define SOS_SIGNAL_H

#include "../lib/types.h"

#define PENDING_SIGNALS_CLEAR 0

#define ALL_SIGNALS_BLOCKED 0
#define ALL_SIGNALS_UNBLOCKED 0xFFFFFFFFFFFFFFFF

typedef void signal_handler();

#define SIGNALS_COUNT 64 // For now just number of bits in pending_signals field

// subset of POSIX signals
typedef enum {
    SIGTEST = 0, // not a posix signal, just for test. TODO: Remove this
    SIGKILL = 9
} signal;

void check_pending_signals();

struct thread;

bool signal_raised(u64 pending_signals, signal sig);
bool signal_allowed(u64 signals_mask, signal sig);
void signal_raise(u64* pending_signals, signal sig);
void signal_clear(u64* pending_signals, signal sig);
void signal_block(u64* signals_mask, signal sig);

#endif // SOS_SIGNAL_H
