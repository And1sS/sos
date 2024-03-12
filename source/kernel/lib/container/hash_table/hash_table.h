#ifndef SOS_HASH_TABLE_H
#define SOS_HASH_TABLE_H

#include "../linked_list/linked_list.h"

typedef struct {
    u64 key;
    void* value;
} hash_table_entry;

typedef struct {
    linked_list* buckets;
    u64 buckets_num;
    u64 size;
} hash_table;

hash_table* hash_table_create();
void hash_table_destroy(hash_table* table);

void hash_table_init(hash_table* table);
void hash_table_deinit(hash_table* table);

bool hash_table_put(hash_table* table, u64 key, void* value, void** old_value);
void* hash_table_get(hash_table* table, u64 key);
void* hash_table_remove(hash_table* table, u64 key);
bool hash_table_contains(hash_table* table, u64 key);

void hash_table_clear(hash_table* table);
void hash_table_print(hash_table* table);

#endif // SOS_HASH_TABLE_H
