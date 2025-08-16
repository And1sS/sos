#ifndef SOS_STRING_H
#define SOS_STRING_H

#include "types.h"

u64 strlen(string str);

string strcpy(string str);

u64 strcmp(string left, string right);

bool streq(string left, string right);

u64 strhash(string str);

#endif // SOS_STRING_H
