#ifndef SOS_HASH_TABLE_H
#define SOS_HASH_TABLE_H

#include "../linked_list/linked_list.h"

#define HASH_TABLE_GET_OR_PUT(table, key, creator)                             \
    ({                                                                         \
        void* ___result = hash_table_get((table), (key));                      \
        if (!___result) {                                                      \
            ___result = (creator);                                             \
            hash_table_put((table), (key), (___result), NULL);                 \
        }                                                                      \
        ___result;                                                             \
    })

typedef u64 hash_function(void* key);
typedef bool key_equal_function(void* key1, void* key2);

typedef struct {
    void* key;
    void* value;
} hash_table_entry;

typedef struct {
    linked_list* buckets;
    u64 buckets_num;
    u64 size;

    hash_function* hasher;
    key_equal_function* comparator;
} hash_table;

hash_table* hash_table_create(hash_function* hasher,
                              key_equal_function* comparator);

void hash_table_destroy(hash_table* table);

bool hash_table_init(hash_table* table, hash_function* hasher,
                     key_equal_function* comparator);
void hash_table_deinit(hash_table* table);

bool hash_table_put(hash_table* table, void* key, void* value,
                    void** old_value);
void* hash_table_get(hash_table* table, void* key);
void* hash_table_remove(hash_table* table, void* key);
bool hash_table_contains(hash_table* table, void* key);

void hash_table_clear(hash_table* table);
void hash_table_print(hash_table* table);

#endif // SOS_HASH_TABLE_H
