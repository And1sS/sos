#include "../../common/idle.h"

void pause() { __asm__ volatile("pause"); }

void halt() { __asm__ volatile("hlt"); }