#ifndef SOS_TAR_H
#define SOS_TAR_H

#include "../types.h"

#define TAR_SECTOR_SIZE 512

typedef enum tar_file_type {
    TAR_NORMAL_FILE = 0,
    TAR_HARD_LINK = 1,
    TAR_SYMBOLIK_LINK = 2,
    TAR_CHARACTER_DEVICE = 3,
    TAR_BLOCK_DEVICE = 4,
    TAR_DIRECTORY = 5,
    TAR_NAMED_PIPE = 6
} tar_file_type;

typedef struct __attribute__((__aligned__(512), __packed__)) {
    char name[100];
    u64 mode;
    u64 uid;
    u64 guid;
    char size[12]; // size is ASCII octal string
    u64 mtime;
    u32 mtime_2;
    u64 checksum;
    u8 type;
    char linked_file_name[100];
    char ustar_indicator[6];
    u16 ustar_version;
    char owner_user_name[32];
    char owner_group_name[32];
    u64 dev_major;
    u64 dev_minor;
    char name_prefix[155];
} tar_header;

typedef struct {
    tar_header header;
    u8 data[];
} tar_entry;

tar_file_type tar_parse_type(tar_entry* entry);

u64 tar_parse_size(tar_entry* entry);

tar_entry* tar_next_entry(tar_entry* entry);

bool tar_is_valid_entry(tar_entry* entry);

void tar_fill_full_entry_name(tar_entry* entry, char name[256]);

void tar_print_entry(tar_entry* entry);

#endif // SOS_TAR_H
