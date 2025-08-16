#include "string.h"
#include "../memory/heap/kheap.h"
#include "memory_util.h"

u64 strlen(string str) {
    u64 len = 0;
    while (str[len] != '\0')
        len++;

    return len;
}

string strcpy(string str) {
    u64 len = strlen(str);

    string copy = kmalloc(sizeof(char) * len);
    memcpy((void*) copy, (void*) str, len);

    return copy;
}

u64 strcmp(string a, string b) {
    const unsigned char* p1 = (const unsigned char*) a;
    const unsigned char* p2 = (const unsigned char*) b;

    while (*p1 && *p1 == *p2)
        ++p1, ++p2;

    return (*p1 > *p2) - (*p2 > *p1);
}

bool streq(string a, string b) { return strcmp(a, b) == 0; }

u64 strhash(string str) {
    u64 hash = 5381;
    u8 c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}