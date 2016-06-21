/**
 * @brief Defines functions used by expand local references when the 4k element
 * reach of normal local references is not enough.
 *
 * @file reference_table.c
 * @author Martin Hagelin
 * @date December, 2014
 *
 */

#include "basic_types.h"
#include "reference_table.h"

/* This type should be defined and present in the type table at all times */

#define DELETED_VALUE (((size_t) 1) << 63 )

typedef struct table_entry {
    uint64_t        key;
    size_t          value;
} __attribute__((packed)) table_entry;


size_t hash_table_size;
size_t hash_table_value_count;
size_t hash_table_del_count;
pool_reference table_pool;
table_entry *reference_table;

/**
 * @brief Copies and cleans up a hash table.
 *
 * @param t_dst the empty table t_src should be copied to.
 * @param t_src the original hash table to copy.
 * @param dst_size the size of the table t_dst.
 * @param src_size the size of the table t_src.
 */
static void
copy_table( table_entry *t_dst,
            table_entry *t_src,
            size_t dst_size,
            size_t src_size);


size_t
expand_local_reference(reference_tag key)
{
    if (0 == key.raw_val)
        return REF_NOT_FOUND;

    if (0 == hash_table_size)
        return REF_NOT_FOUND;

    size_t idx = hash_func(key.raw_val) % hash_table_size;
    for (size_t i = 0 ; i < hash_table_size ; ++i) {
        uint64_t tag = reference_table[idx].key;
        size_t value = reference_table[idx].value;

        if (tag == key.raw_val)
            return value & ~DELETED_VALUE;

        if (value == 0)
            break;

        idx = idx == hash_table_size -1? 0 : idx + 1;
    }

    return REF_NOT_FOUND;
}

int
compress_absolute_index(reference_tag key, size_t value)
{
    if (hash_table_value_count*2 >= hash_table_size) {
        size_t new_size = hash_table_size? hash_table_size*2 : PAGE_SIZE;
        assert(0 == grow_hash_table(new_size));
    } else if ((hash_table_value_count+hash_table_del_count)*2 >= 
                hash_table_size) {
        cleanup_hash_table();
    }

    if (0 == key.raw_val)
        return 1;

    if (value & DELETED_VALUE)
        return 1;

    size_t idx = hash_func(key.raw_val) % hash_table_size;

    size_t first_free_spot = REF_NOT_FOUND;
    for (size_t i = 0 ; 1 ; ++i) {
        uint64_t tag = reference_table[idx].key;
        size_t val = reference_table[idx].value;

        if (tag == key.raw_val) {
            first_free_spot = idx;
            break;
        }

        if (0 == tag && first_free_spot == REF_NOT_FOUND)
            first_free_spot = idx;

        if (first_free_spot != REF_NOT_FOUND && !(val & DELETED_VALUE))
            break;

        idx = idx == hash_table_size -1? 0 : idx + 1;
    }

    if (0 == reference_table[first_free_spot].key) {
        if (DELETED_VALUE & reference_table[first_free_spot].value)
            hash_table_del_count--;
        hash_table_value_count++;
    }

    reference_table[first_free_spot].key = key.raw_val;
    reference_table[first_free_spot].value = DELETED_VALUE | value;


    return 0;
}

int
delete_reference(reference_tag key)
{
    if (0 == key.raw_val)
        return 1;

    if (0 == hash_table_size)
        return 1;

    size_t idx = hash_func(key.raw_val) % hash_table_size;
    for (size_t i = 0 ; i < hash_table_size ; ++i) {
        uint64_t tag = reference_table[idx].key;
        size_t value = reference_table[idx].value;

        if (tag == key.raw_val) {
            reference_table[idx].value = DELETED_VALUE;
            reference_table[idx].key = 0;
            hash_table_value_count--;
            hash_table_del_count++;
            return 0;
        }

        if (value == 0)
            break;

        idx = idx == hash_table_size -1? 0 : idx + 1;
    }

    return 1;
}

int
delete_all_for_pool(pool_reference pool)
{
    pool_struct p = {.raw_val = pool};
    uint16_t pool_id = p.pool_id;

    if (0 == pool_id)
        return 1;
    /* 
     * The fact that a hash table is used makes calling this function quite
     * expensive. If this becomes a problem it might be worthwhile adding a
     * table for each pool with expanded references.
     */

    /*
     * TODO:
     * Since the entire structure is looped through, this would be a good
     * opportunity to clean the table as well.
     */
    for (size_t i = 0 ; i < hash_table_size ; ++i) {
        reference_tag t = {.raw_val = reference_table[i].key};
        if (t.pool_id == pool_id) {
            reference_table[i].key = 0;
            reference_table[i].value = DELETED_VALUE;
            hash_table_value_count--;
            hash_table_del_count++;
        }
    }
    return 0;
}

PRIVATE uint64_t
hash_func(uint64_t key)
{
    key = ~key + (key << 21);
    key = key ^ (key >> 24);
    key = (key + (key <<3)) + (key << 8);
    key ^= key >> 14;
    key = (key + (key << 2)) + (key << 4);
    key ^= key >> 28;
    key += key << 31;

    return key;
}

PRIVATE void
copy_table( table_entry *t_dst, 
            table_entry *t_src,
            size_t dst_size,
            size_t src_size)
{
    for (size_t i = 0 ; i < src_size ; ++i) {
        if (0 != t_src[i].key) {
            uint64_t hash_val = hash_func(t_src[i].key);

            /* Simply using linear probing for now */
            size_t idx = hash_val % dst_size;
            for (size_t j = 0 ; j < dst_size; ++j) {

                if (!t_dst[idx].key) {
                    t_dst[idx] = t_src[i];
                    break;
                }

                idx = idx == dst_size -1? 0 : idx + 1;
            }
        }
    }
}

PRIVATE int
grow_hash_table(const size_t new_size)
{
    /* TODO Insert locks */

    pool_reference new_pool = pool_create(REFERENCE_TABLE_ENTRY);
    if (NULL_POOL == new_pool)
        return 1;

    if (0 != pool_grow(&new_pool, new_size))
        return 1;

    table_entry *new_table = pool_to_array(new_pool);

    copy_table(new_table, reference_table, new_size, hash_table_size);

    pool_destroy(&table_pool);

    reference_table = new_table;
    table_pool = new_pool;
    hash_table_size = new_size;
    hash_table_del_count = 0;


    return 0;
}

PRIVATE int
cleanup_hash_table(void)
{
    /* TODO Fallback version for when a copy doesn't fit on the stack */

    table_entry clean_copy[hash_table_size];
    memset(clean_copy, 0, sizeof(table_entry)*hash_table_size);
    copy_table(clean_copy, reference_table, hash_table_size, hash_table_size);
    memcpy(reference_table, clean_copy, sizeof(table_entry)*hash_table_size);

    hash_table_del_count = 0;
    return 0;
}

