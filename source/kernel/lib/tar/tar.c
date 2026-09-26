#include "tar.h"
#include "../alignment.h"
#include "../kprint.h"
#include "../memory_util.h"
#include "../string.h"

tar_file_type tar_parse_type(tar_entry* entry) {
    return entry->header.type - '0';
}

static u64 parse_tar_size(string field) {
    unsigned char c = field[0];

    /* GNU base-256 encoding (large files) */
    if (c & 0x80) {
        u64 v = c & 0x7F; /* drop the indicator bit */
        for (int i = 1; i < 12; i++)
            v = (v << 8) | field[i];
        return v;
    }

    /* Standard octal, possibly padded with leading spaces and terminated
       by space or NUL. */
    int i = 0;
    while (i < 12 && field[i] == ' ')
        i++;

    u64 v = 0;
    while (i < 12) {
        c = field[i];
        if (c == '\0' || c == ' ')
            break;
        if (c < '0' || c > '7')
            return 0; /* malformed */
        v = (v << 3) | (c - '0');
        i++;
    }
    return v;
}

u64 tar_parse_size(tar_entry* entry) {
    return parse_tar_size((const char*) entry->header.size);
}

tar_entry* tar_next_entry(tar_entry* entry) {
    u64 next_entry = (u64) entry + sizeof(tar_header)
                     + align_to_upper(tar_parse_size(entry), TAR_SECTOR_SIZE);

    return (tar_entry*) next_entry;
}

bool tar_is_valid_entry(tar_entry* entry) {
    static string ustar_indicator = "ustar ";

    for (u64 i = 0; i < 6; i++) {
        if (entry->header.ustar_indicator[i] != ustar_indicator[i])
            return false;
    }
    return true;
}

void tar_fill_full_entry_name(tar_entry* entry, char name[256]) {
    u64 name_len = strlenn(entry->header.name, 100);
    u64 prefix_len = strlenn(entry->header.name_prefix, 155);
    if (prefix_len > 0) {
        memcpy(name, entry->header.name_prefix, prefix_len);
        name[prefix_len] = '/';
        memcpy(name + prefix_len + 1, entry->header.name, name_len + 1);
    } else {
        memcpy(name, entry->header.name, name_len + 1);
    }

    name[255] = '\0';
}

void tar_print_entry(tar_entry* entry) {
    tar_file_type type = tar_parse_type(entry);

    print(entry->header.name);
    print(", t: ");
    if (type == TAR_NORMAL_FILE)
        print("file");
    else if (type == TAR_DIRECTORY)
        print("dir");
    else
        print("unrecognized");

    print(", s: ");
    print_u64(tar_parse_size(entry));
    println("b");
}
