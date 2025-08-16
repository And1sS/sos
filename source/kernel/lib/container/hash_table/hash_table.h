#ifndef SOS_HASH_TABLE_H
#define SOS_HASH_TABLE_H

#include "../../kprint.h"
#include "../../util.h"
#include "../linked_list/linked_list.h"

typedef u64 hasher(void* key);
typedef bool comparator(void* a, void* b);

#define INITIAL_BUCKETS_NUM 16

#define DEFINE_HASH_TABLE_NO_MM(name, key_type, value_type, hasher,            \
                                comparator)                                    \
    static void ___do_nothing_##name(key_type key) { UNUSED(key); }            \
    static void ___do_nothing_##name##1(value_type value) { UNUSED(value); }   \
    DEFINE_HASH_TABLE(name, key_type, value_type, hasher, comparator,          \
                      ___do_nothing_##name, ___do_nothing_##name##1)

#define DEFINE_HASH_TABLE(name, key_type, value_type, hasher, comparator,      \
                          key_destroyer, value_destroyer)                      \
                                                                               \
    _Pragma("GCC diagnostic push")                                             \
        _Pragma("GCC diagnostic ignored \"-Wunused-function\"")                \
                                                                               \
            typedef struct {                                                   \
        key_type key;                                                          \
        value_type value;                                                      \
    } name##_entry;                                                            \
                                                                               \
    typedef struct {                                                           \
        linked_list* buckets;                                                  \
        u64 buckets_num;                                                       \
        u64 size;                                                              \
    } name;                                                                    \
                                                                               \
    static bool name##_grow(name* table) {                                     \
        u64 new_buckets_num = table->buckets_num * 2;                          \
        linked_list* new_buckets =                                             \
            (linked_list*) kmalloc(new_buckets_num * sizeof(linked_list));     \
                                                                               \
        if (!new_buckets)                                                      \
            return false;                                                      \
                                                                               \
        for (u64 i = 0; i < new_buckets_num; i++) {                            \
            linked_list_init(&new_buckets[i]);                                 \
        }                                                                      \
                                                                               \
        for (u64 i = 0; i < table->buckets_num; i++) {                         \
            linked_list* bucket = &table->buckets[i];                          \
            linked_list_node* entry_node = bucket->head;                       \
            while (entry_node != NULL) {                                       \
                name##_entry* entry = (name##_entry*) entry_node->value;       \
                linked_list_node* crt_node = entry_node;                       \
                entry_node = crt_node->next;                                   \
                linked_list_remove_node(bucket, crt_node);                     \
                                                                               \
                key_type key = entry->key;                                     \
                linked_list_add_last_node(                                     \
                    &new_buckets[hasher(key) % new_buckets_num], crt_node);    \
            }                                                                  \
        }                                                                      \
                                                                               \
        kfree(table->buckets);                                                 \
                                                                               \
        table->buckets = new_buckets;                                          \
        table->buckets_num = new_buckets_num;                                  \
        return true;                                                           \
    }                                                                          \
                                                                               \
    bool name##_init(name* table) {                                            \
        table->size = 0;                                                       \
        table->buckets_num = INITIAL_BUCKETS_NUM;                              \
        table->buckets =                                                       \
            (linked_list*) kmalloc(table->buckets_num * sizeof(linked_list));  \
                                                                               \
        if (!table->buckets)                                                   \
            return false;                                                      \
                                                                               \
        for (u64 i = 0; i < table->buckets_num; i++) {                         \
            linked_list_init(&table->buckets[i]);                              \
        }                                                                      \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    static name* name##_create() {                                             \
        name* table = kmalloc(sizeof(name));                                   \
        if (!table)                                                            \
            return NULL;                                                       \
                                                                               \
        name##_init(table);                                                    \
        return table;                                                          \
    }                                                                          \
                                                                               \
    void name##_deinit(name* table) {                                          \
        for (u64 i = 0; i < table->buckets_num; i++) {                         \
            linked_list* bucket = &table->buckets[i];                          \
            linked_list_node* entry_node = bucket->head;                       \
                                                                               \
            while (entry_node != NULL) {                                       \
                name##_entry* entry = (name##_entry*) entry_node->value;       \
                key_destroyer(entry->key);                                     \
                value_destroyer(entry->value);                                 \
                kfree(entry);                                                  \
                linked_list_node* crt_node = entry_node;                       \
                entry_node = crt_node->next;                                   \
                                                                               \
                linked_list_remove_node(bucket, crt_node);                     \
                kfree(crt_node);                                               \
            }                                                                  \
        }                                                                      \
                                                                               \
        kfree(table->buckets);                                                 \
        table->buckets = NULL;                                                 \
        table->buckets_num = 0;                                                \
        table->size = 0;                                                       \
    }                                                                          \
                                                                               \
    void name##_destroy(name* table) {                                         \
        name##_deinit(table);                                                  \
        kfree(table);                                                          \
    }                                                                          \
                                                                               \
    void name##_clear(name* table) {                                           \
        for (u64 i = 0; i < table->buckets_num; i++) {                         \
            linked_list* bucket = &table->buckets[i];                          \
            linked_list_node* entry_node = bucket->head;                       \
                                                                               \
            while (entry_node != NULL) {                                       \
                name##_entry* entry = (name##_entry*) entry_node->value;       \
                key_destroyer(entry->key);                                     \
                value_destroyer(entry->value);                                 \
                kfree(entry);                                                  \
                linked_list_node* current_node = entry_node;                   \
                entry_node = current_node->next;                               \
                                                                               \
                linked_list_remove_node(bucket, current_node);                 \
                kfree(current_node);                                           \
            }                                                                  \
        }                                                                      \
                                                                               \
        table->size = 0;                                                       \
    }                                                                          \
                                                                               \
    static linked_list_node* name##_bucket_lookup(linked_list* bucket,         \
                                                  key_type key) {              \
                                                                               \
        linked_list_node* entry_node = bucket->head;                           \
        while (entry_node != NULL) {                                           \
            name##_entry* entry = (name##_entry*) entry_node->value;           \
            if (comparator(entry->key, key))                                   \
                return entry_node;                                             \
                                                                               \
            entry_node = entry_node->next;                                     \
        }                                                                      \
                                                                               \
        return NULL;                                                           \
    }                                                                          \
                                                                               \
    value_type name##_get(name* table, key_type key) {                         \
        linked_list* bucket =                                                  \
            &table->buckets[hasher(key) % table->buckets_num];                 \
        linked_list_node* entry_node = name##_bucket_lookup(bucket, key);      \
        return entry_node ? ((name##_entry*) entry_node->value)->value : NULL; \
    }                                                                          \
                                                                               \
    bool name##_contains(name* table, key_type key) {                          \
        linked_list* bucket =                                                  \
            &table->buckets[hasher(key) % table->buckets_num];                 \
        return name##_bucket_lookup(bucket, key) != NULL;                      \
    }                                                                          \
                                                                               \
    bool name##_put(name* table, key_type key, value_type value,               \
                    value_type* old_value) {                                   \
                                                                               \
        if ((table->size + 1) * 100 > table->buckets_num * 75                  \
            && !name##_grow(table))                                            \
            return false;                                                      \
                                                                               \
        linked_list* bucket =                                                  \
            &table->buckets[hasher(key) % table->buckets_num];                 \
        linked_list_node* entry_node = name##_bucket_lookup(bucket, key);      \
        if (entry_node) {                                                      \
            name##_entry* entry = (name##_entry*) entry_node->value;           \
            if (old_value) {                                                   \
                *old_value = entry->value;                                     \
            } else {                                                           \
                value_destroyer(entry->value);                                 \
            }                                                                  \
            entry->value = value;                                              \
            return true;                                                       \
        }                                                                      \
                                                                               \
        name##_entry* new_entry =                                              \
            (name##_entry*) kmalloc(sizeof(name##_entry));                     \
        new_entry->key = key;                                                  \
        new_entry->value = value;                                              \
        if (!linked_list_add_last(bucket, new_entry)) {                        \
            kfree(new_entry);                                                  \
            return false;                                                      \
        }                                                                      \
                                                                               \
        if (old_value) {                                                       \
            *old_value = NULL;                                                 \
        }                                                                      \
        table->size++;                                                         \
        return true;                                                           \
    }                                                                          \
                                                                               \
    value_type name##_remove(name* table, key_type key) {                      \
        linked_list* bucket =                                                  \
            &table->buckets[hasher(key) % table->buckets_num];                 \
        linked_list_node* entry_node = name##_bucket_lookup(bucket, key);      \
                                                                               \
        if (!entry_node)                                                       \
            return NULL;                                                       \
                                                                               \
        linked_list_remove_node(bucket, entry_node);                           \
        name##_entry* entry = (name##_entry*) entry_node->value;               \
        value_type value = entry->value;                                       \
        kfree(entry);                                                          \
        kfree(entry_node);                                                     \
        table->size--;                                                         \
        return value;                                                          \
    }                                                                          \
                                                                               \
    void name##_print(name* table) {                                           \
        println("hash table {");                                               \
        print("size = ");                                                      \
        print_u64(table->size);                                                \
        print("; ");                                                           \
                                                                               \
        for (u64 i = 0; i < table->buckets_num; i++) {                         \
            linked_list* bucket = &table->buckets[i];                          \
            linked_list_node* entry_node = bucket->head;                       \
            while (entry_node != NULL) {                                       \
                name##_entry* entry = (name##_entry*) entry_node->value;       \
                print("key = ");                                           \
                print_u64((u64) entry->key);                                   \
                print(", value = ");                                           \
                print_u64((u64) entry->value);                                 \
                print("; ");                                                   \
                entry_node = entry_node->next;                                 \
            }                                                                  \
        }                                                                      \
                                                                               \
        print("}");                                                          \
    }                                                                          \
    _Pragma("GCC diagnostic pop")

#endif // SOS_HASH_TABLE_H
