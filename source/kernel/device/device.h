#ifndef SOS_DEVICE_H
#define SOS_DEVICE_H

#include "../lib/types.h"

struct device;

typedef struct {
    u64 (*open)(struct device* device, u64 flags);
    u64 (*close)(struct device* device);
    u64 (*read)(struct device* device, u64 off, u64 nmax, u8* buffer);
    u64 (*write)(struct device* device, u64 off, u64 nmax, u8* buffer);
    u64 (*d_ioctl)(struct device* device, u64 cmd, void* arg);
} device_ops;

typedef struct {
    u64 id;

    void* private_data;
    device_ops* ops;
} device;

#endif // SOS_DEVICE_H
