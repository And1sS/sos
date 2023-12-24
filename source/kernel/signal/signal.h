#ifndef SOS_SIGNAL_H
#define SOS_SIGNAL_H

#include "../lib/types.h"
#include "../threading/thread.h"

#define PENDING_SIGNALS_CLEAR 0

#define ALL_SIGNALS_BLOCKED 0
#define ALL_SIGNALS_UNBLOCKED 0xFFFFFFFFFFFFFFFF

// subset of POSIX signals
typedef enum { SIGTEST = 0, SIGKILL = 9 } signal;

void check_pending_signals();

// this one should be called with threading lock held
void block_and_clear_all_signals(thread* thrd);

bool signal_thread(thread* thrd, signal sig);

#endif // SOS_SIGNAL_H
