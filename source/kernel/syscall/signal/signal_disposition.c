#include "../../error/errno.h"
#include "../../lib/types.h"
#include "../../lib/util.h"
#include "../../signal/signal.h"
#include "../../threading/process.h"

u64 sys_set_signal_disposition(u64 arg0, u64 arg1,
                               struct cpu_context* context) {

    UNUSED(context);

    signal sig = arg0;
    signal_disposition disposition = arg1;

    return process_set_signal_disposition(sig, disposition) ? 0 : -EPERM;
}