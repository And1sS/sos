#ifndef SOS_USER_SIGNAL_H
#define SOS_USER_SIGNAL_H

typedef void signal_handler();

typedef enum {
    FALLBACK_TO_DEFAULT = 0,
    TERMINATE = 1,
    IGNORE = 2,
    CORE_DUMP = 3,
    STOP = 4,
    CONTINUE = 5
} signal_disposition;

typedef enum {
    SIGHUP = 1,
    SIGINT = 2,
    SIGQUIT = 3,
    SIGILL = 4,
    SIGABRT = 6,
    SIGKILL = 9,
    SIGSEGV = 11,
    SIGTERM = 14
} signal;

typedef struct {
    signal_disposition disposition;
    signal_handler* handler;
} sigaction;

long set_sigaction(signal sig, const sigaction* action);

#endif // SOS_USER_SIGNAL_H
