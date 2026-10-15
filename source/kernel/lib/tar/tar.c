#include "tar.h"
#include "../alignment.h"
#include "../kprint.h"
#include "../math.h"
#include "../memory_util.h"
#include "../string.h"
#include "../util.h"

tar_file_type tar_parse_type(tar_entry* entry) {
    return entry->header.type - '0';
}

u64 tar_parse_size(tar_entry* entry) {
    char* iter = entry->header.size;
    char* end = iter + MEMBER_SIZE(tar_header, size);

    u64 size = 0;
    for (; iter != end && *iter; iter++)
        size = size * 8 + MAX(*iter, '0') - '0'; // max for skipping spaces

    return size;
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

void tar_fill_full_entry_name(tar_entry* entry, char name[TAR_NAME_MAX]) {
    u64 name_len = strlenn(entry->header.name, MEMBER_SIZE(tar_header, name));
    u64 prefix_len = strlenn(entry->header.name_prefix,
                             MEMBER_SIZE(tar_header, name_prefix));

    if (prefix_len > 0) {
        memcpy(name, entry->header.name_prefix, prefix_len);
        name[prefix_len] = '/';
        memcpy(name + prefix_len + 1, entry->header.name, name_len + 1);
    } else {
        memcpy(name, entry->header.name, name_len + 1);
    }

    name[TAR_NAME_MAX - 1] = '\0';
}

void tar_print_entry(tar_entry* entry) {
    tar_file_type type = tar_parse_type(entry);

    print(entry->header.name);
    print(", t: ");
    if (type == TAR_FILE)
        print("file");
    else if (type == TAR_DIRECTORY)
        print("dir");
    else
        print("unrecognized");

    print(", s: ");
    print_u64(tar_parse_size(entry));
    println("b");
}
