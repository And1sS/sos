#include "string.h"

u64 strlen(string str) {
    u64 len = 0;
    while (str[len++] != '\0')
        ;
    return len;
}

bool strcmp(string a, string b) {
    const unsigned char* p1 = (const unsigned char*) a;
    const unsigned char* p2 = (const unsigned char*) b;

    while (*p1 && *p1 == *p2)
        ++p1, ++p2;

    return (*p1 > *p2) - (*p2 > *p1);
}

u64 strhash(string str) {
    u64 hash = 5381;
    u8 c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}