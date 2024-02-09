#ifndef SOS_PIC_H
#define SOS_PIC_H

#include "../../../lib/types.h"

typedef enum {
    NON_BUFFERED = 0,
    SLAVE_BUFFERED_MODE = 0b10,
    MASTER_BUFFERED_MODE = 0b11
} pic_buffered_mode;

void pic_init(void);
void pic_ack(u8 irq_num);

#endif // SOS_PIC_H