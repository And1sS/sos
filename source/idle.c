#include "idle.h"

void pause() { __asm__ volatile("pause"); }