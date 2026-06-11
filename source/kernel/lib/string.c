#include "string.h"
#include "../error/errno.h"
#include "../error/error.h"
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
    char* copy = kmalloc(sizeof(char) * (len + 1));
    if (!copy)
        return NULL;

    memcpy((void*) copy, (void*) str, len);
    copy[len] = '\0';

    return copy;
}

string strcpyn(string str, u64 limit) {
    u64 len = 0;
    // first check that we have not exceeded the limit
    while (len < limit && str[len] != '\0') {
        // we have reached the limit and did not find end of the string
        if (++len == limit)
            return ERROR_PTR(-EFAULT);
    }

    if (!len)
        return ERROR_PTR(-EFAULT);

    char* copy = kmalloc(sizeof(char) * (len + 1));
    if (!copy)
        return ERROR_PTR(-ENOMEM);

    memcpy((void*) copy, (void*) str, len);
    copy[len] = '\0';

    return copy;
}

void strfree(string str) { kfree((void*) str); }

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