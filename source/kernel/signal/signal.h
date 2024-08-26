#ifndef SOS_SIGNAL_H
#define SOS_SIGNAL_H

#include "../lib/types.h"

/*
 * POSIX signals
 *
 * First the signals described in the original POSIX.1-1990 standard.
 *
 * Signal     Value     Action   Comment
 * ───────────────────────────────────
 * SIGHUP        1       Term    Hangup detected on controlling terminal
 *                               or death of controlling process
 * SIGINT        2       Term    Interrupt from keyboard
 * SIGQUIT       3       Core    Quit from keyboard
 * SIGILL        4       Core    Illegal Instruction
 * SIGABRT       6       Core    Abort signal from abort(3)
 * SIGFPE        8       Core    Floating point exception
 * SIGKILL       9       Term    Kill signal
 * SIGSEGV      11       Core    Invalid memory reference
 * SIGPIPE      13       Term    Broken pipe: write to pipe with no
 *                               readers
 * SIGALRM      14       Term    Timer signal from alarm(2)
 * SIGTERM      15       Term    Termination signal
 * SIGUSR1   30,10,16    Term    User-defined signal 1
 * SIGUSR2   31,12,17    Term    User-defined signal 2
 * SIGCHLD   20,17,18    Ign     Child stopped or terminated
 * SIGCONT   19,18,25    Cont    Continue if stopped
 * SIGSTOP   17,19,23    Stop    Stop process
 * SIGTSTP   18,20,24    Stop    Stop typed at terminal
 * SIGTTIN   21,21,26    Stop    Terminal input for background process
 * SIGTTOU   22,22,27    Stop    Terminal output for background process
 *
 *
 * Signal dispositions
 * Each signal has a current disposition, which determines how the
 * process behaves when it is delivered the signal.
 *
 * The entries in the "Action" column of the table below specify the
 * default disposition for each signal, as follows:
 *
 * Term   Default action is to terminate the process.
 *
 * Ign    Default action is to ignore the signal.
 *
 * Core   Default action is to terminate the process and dump core.
 *
 * Stop   Default action is to stop the process.
 *
 * Cont   Default action is to continue the process if it is
 *        currently stopped.
 */

#define PENDING_SIGNALS_CLEAR 0

#define ALL_SIGNALS_BLOCKED 0
#define ALL_SIGNALS_UNBLOCKED 0xFFFFFFFFFFFFFFFF

typedef u64 sigmask;    // bitmap of blocked signals, 0 - signal blocked,
                        // 1 - signal unblocked
typedef u64 sigpending; // bitmap of pending signals

typedef void signal_handler();

#define SIGNALS_COUNT 31

typedef enum {
    FALLBACK_TO_DEFAULT = 0,
    TERMINATE = 1,
    IGNORE = 2,
    CORE_DUMP = 3,
    STOP = 4,
    CONTINUE = 5
} signal_disposition;

extern signal_disposition default_dispositions[SIGNALS_COUNT + 1];

// For now only limited amount of signals is supported
typedef enum {
    NOSIG = 0,
    SIGHUP = 1,
    SIGINT = 2,
    SIGQUIT = 3,
    SIGILL = 4,
    SIGABRT = 6,
    SIGKILL = 9,
    SIGSEGV = 11,
    SIGTERM = 14,
    SIGCHLD = 20
} signal;

typedef struct {
    signal_disposition disposition;
    signal_handler* handler;
} sigaction;

struct cpu_context;

void check_pending_signals(struct cpu_context* context);

bool signal_raised(sigpending pending_signals, signal sig);
bool signal_any_raised(sigpending pending_signals);
bool signal_allowed(sigmask signals_mask, signal sig);
void signal_raise(sigpending* pending_signals, signal sig);
void signal_clear(sigpending* pending_signals, signal sig);
void signal_block(sigmask* signals_mask, signal sig);
void signal_unblock(sigmask* signals_mask, signal sig);

#endif // SOS_SIGNAL_H
