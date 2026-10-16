#ifndef SOS_STRING_H
#define SOS_STRING_H

#include "types.h"

u64 strlen(string str);

// returns either pointer to string, or NULL when not enough memory
string strcpy(string str);

// returns pointer to string when everything is ok,
// EFAULT when null terminator hasn't been found under the limit or string is
// empty,
// ENOMEM when not enough memory
string strcpyn(string str, u64 limit);

void strfree(string str);

u64 strcmp(string left, string right);

bool streq(string left, string right);

u64 strhash(string str);

#endif // SOS_STRING_H
