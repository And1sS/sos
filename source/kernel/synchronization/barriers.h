#ifndef SOS_BARRIERS_H
#define SOS_BARRIERS_H

#define barrier() __asm__ __volatile__("" ::: "memory")
#define barrier_data(ptr) __asm__ __volatile__("" ::"r"(ptr) : "memory")

extern void smp_mb();

extern void smp_wb();

extern void smp_rb();

#endif // SOS_BARRIERS_H
