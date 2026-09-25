#ifndef SOS_INIT_H
#define SOS_INIT_H

#include "multiboot.h"

void set_up_init_fs(module initramfs_module);
void set_up_init_process();

#endif // SOS_INIT_H
