#include "hash_table.h"
#include "../../../memory/heap/kheap.h"
#include "../../kprint.h"

#define INITIAL_BUCKETS_NUM 16

static bool hash_table_grow(hash_table* table) {
    u64 new_buckets_num = table->buckets_num * 2;
    linked_list* new_buckets = kmalloc(new_buckets_num * sizeof(linked_list));

    if (!new_buckets)
        return false;

    for (u64 i = 0; i < new_buckets_num; i++) {
        linked_list_init(&new_buckets[i]);
    }

    for (u64 i = 0; i < table->buckets_num; i++) {
        linked_list* bucket = &table->buckets[i];
        linked_list_node* entry_node = bucket->head;
        while (entry_node != NULL) {
            hash_table_entry* entry = entry_node->value;
            linked_list_node* crt_node = entry_node;
            entry_node = crt_node->next;
            linked_list_remove_node(bucket, crt_node);

            u64 hash = table->hasher(entry->key);
            linked_list_add_last_node(&new_buckets[hash % new_buckets_num],
                                      crt_node);
        }
    }

    kfree(table->buckets);

    table->buckets = new_buckets;
    table->buckets_num = new_buckets_num;
    return true;
}

hash_table* hash_table_create(hash_function* hasher,
                              key_equal_function* comparator) {

    hash_table* table = kmalloc(sizeof(hash_table));
    if (!table || !hash_table_init(table, hasher, comparator)) {
        kfree(table);
        return NULL;
    }

    return table;
}

void hash_table_destroy(hash_table* table) {
    hash_table_deinit(table);
    kfree(table);
}

bool hash_table_init(hash_table* table, hash_function* hasher,
                     key_equal_function* comparator) {

    table->size = 0;
    table->buckets_num = INITIAL_BUCKETS_NUM;
    table->buckets =
        (linked_list*) kmalloc(table->buckets_num * sizeof(linked_list));
    table->hasher = hasher;
    table->comparator = comparator;

    if (!table->buckets)
        return false;

    for (u64 i = 0; i < table->buckets_num; i++) {
        linked_list_init(&table->buckets[i]);
    }

    return true;
}

void hash_table_deinit(hash_table* table) {
    hash_table_clear(table);
    kfree(table->buckets);
    table->buckets = NULL;
    table->buckets_num = 0;
}

void hash_table_clear(hash_table* table) {
    for (u64 i = 0; i < table->buckets_num; i++) {
        linked_list* bucket = &table->buckets[i];
        linked_list_node* entry_node = bucket->head;

        while (entry_node != NULL) {
            hash_table_entry* entry = entry_node->value;
            kfree(entry);
            linked_list_node* current_node = entry_node;
            entry_node = current_node->next;

            linked_list_remove_node(bucket, current_node);
            kfree(current_node);
        }
    }

    table->size = 0;
}

static linked_list_node* hash_table_bucket_lookup(
    linked_list* bucket, void* key, key_equal_function* comparator) {

    linked_list_node* entry_node = bucket->head;
    while (entry_node != NULL) {
        hash_table_entry* entry = entry_node->value;
        if (comparator(entry->key, key))
            return entry_node;

        entry_node = entry_node->next;
    }

    return NULL;
}

void* hash_table_get(hash_table* table, void* key) {
    linked_list* bucket =
        &table->buckets[table->hasher(key) % table->buckets_num];

    linked_list_node* entry_node =
        hash_table_bucket_lookup(bucket, key, table->comparator);

    return entry_node ? ((hash_table_entry*) entry_node->value)->value : NULL;
}

bool hash_table_contains(hash_table* table, void* key) {
    linked_list* bucket =
        &table->buckets[table->hasher(key) % table->buckets_num];
    return hash_table_bucket_lookup(bucket, key, table->comparator) != NULL;
}

bool hash_table_put(hash_table* table, void* key, void* value,
                    void** old_value) {

    // table->size + 1 > table->buckets_num * 0.75, where 0.75 is load factor
    if ((table->size + 1) * 100 > table->buckets_num * 75
        && !hash_table_grow(table))
        return false;

    linked_list* bucket =
        &table->buckets[table->hasher(key) % table->buckets_num];

    linked_list_node* entry_node =
        hash_table_bucket_lookup(bucket, key, table->comparator);

    if (entry_node) {
        hash_table_entry* entry = entry_node->value;
        if (old_value) {
            *old_value = entry->value;
        }
        entry->value = value;
        return true;
    }

    hash_table_entry* new_entry = kmalloc(sizeof(hash_table_entry));
    new_entry->key = key;
    new_entry->value = value;
    if (!linked_list_add_last(bucket, new_entry)) {
        kfree(new_entry);
        return false;
    }

    if (old_value) {
        *old_value = NULL;
    }
    table->size++;
    return true;
}

void* hash_table_remove(hash_table* table, void* key) {
    linked_list* bucket =
        &table->buckets[table->hasher(key) % table->buckets_num];

    linked_list_node* entry_node =
        hash_table_bucket_lookup(bucket, key, table->comparator);

    if (!entry_node)
        return NULL;

    linked_list_remove_node(bucket, entry_node);
    hash_table_entry* entry = entry_node->value;
    void* value = entry->value;
    kfree(entry);
    kfree(entry_node);
    table->size--;
    return value;
}

void hash_table_print(hash_table* table) {
    println("hash table {");
    print("size = ");
    print_u64(table->size);
    println("");

    for (u64 i = 0; i < table->buckets_num; i++) {
        linked_list* bucket = &table->buckets[i];
        linked_list_node* entry_node = bucket->head;
        while (entry_node != NULL) {
            hash_table_entry* entry = entry_node->value;
            print("    key = ");
            print_u64((u64) entry->key);
            print(", value = ");
            print_u64((u64) entry->value);
            println("");
            entry_node = entry_node->next;
        }
    }

    println("}");
}