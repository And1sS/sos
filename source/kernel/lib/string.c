#include "string.h"

u64 strlen(string str) {
    u64 len = 0;
    while (str[len++] != '\0')
        ;
    return len;
}

u64 strhash(string str) {
    u64 hash = 5381;
    u8 c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}