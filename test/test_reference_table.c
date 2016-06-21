#include "test_reference_table.h"

void
t_expand_and_compress_local_reference(void)
{
    reference_tag tag = {.raw_val = 0xbeefdeadbeef };
    size_t abs_idx = 424242;

    reference_tag null_tag = {.raw_val = 0};

    CU_ASSERT_EQUAL(expand_local_reference(tag), REF_NOT_FOUND);
    CU_ASSERT_NOT_EQUAL(compress_absolute_index(tag, REF_NOT_FOUND), 0);
    CU_ASSERT_NOT_EQUAL(compress_absolute_index(null_tag, abs_idx), 0);
    CU_ASSERT_EQUAL(expand_local_reference(null_tag), REF_NOT_FOUND);

    CU_ASSERT_EQUAL(compress_absolute_index(tag, abs_idx), 0);

    size_t expanded_idx = expand_local_reference(tag);
    CU_ASSERT_NOT_EQUAL(expanded_idx, REF_NOT_FOUND);
    CU_ASSERT_EQUAL(expanded_idx, abs_idx);

    int insert_errors = 0;
    for (int i = 0 ; i < 1000 ; ++i) {
        reference_tag t = {.raw_val = 0xbeef00000000 + i};
        insert_errors += compress_absolute_index(t, (size_t) i) != 0;
    }
    CU_ASSERT_EQUAL(insert_errors, 0);

    int get_errors = 0;
    for (int i = 0 ; i < 1000 ; ++i) {
        reference_tag t = {.raw_val = 0xbeef00000000 + i};
        get_errors = expand_local_reference(t) != (size_t) i;
    }
    CU_ASSERT_EQUAL(get_errors, 0);
}

void
t_delete_reference(void)
{
    reference_tag tag_0 = {.raw_val = 0};
    reference_tag tag_1 = {.raw_val = 0xbabe04040404}; 
    reference_tag tag_2 = {.raw_val = 0xbeefdeadbeef}; 

    CU_ASSERT_NOT_EQUAL(delete_reference(tag_0), 0);
    CU_ASSERT_NOT_EQUAL(delete_reference(tag_1), 0);

    CU_ASSERT_EQUAL(delete_reference(tag_2), 0);
    CU_ASSERT_NOT_EQUAL(delete_reference(tag_2), 0);

    extern size_t hash_table_size;

    size_t old_size = hash_table_size;

    int insert_errors = 0;
    int get_errors = 0;
    int delete_errors = 0;

    for (size_t i = 0 ; i < old_size*10; ++i ) {
        insert_errors += compress_absolute_index(tag_2, 42) != 0;
        get_errors += expand_local_reference(tag_2) != (size_t) 42;
        delete_errors += delete_reference(tag_2) != 0;
        delete_errors += expand_local_reference(tag_2) != REF_NOT_FOUND;
    }
    CU_ASSERT_EQUAL(hash_table_size, old_size);
    CU_ASSERT_EQUAL(insert_errors, 0);
    CU_ASSERT_EQUAL(get_errors, 0);
    CU_ASSERT_EQUAL(delete_errors, 0);
}

void
t_cleanup_hash_table(void)
{
    extern size_t hash_table_size;
    size_t old_size = hash_table_size;

    int insert_errors = 0;
    int get_errors = 0;
    int delete_errors = 0;
    for (size_t i = 0 ; i < old_size*10 ; ++i ){
        reference_tag t = {.raw_val = 0xbabe00000000 + i};
        insert_errors += compress_absolute_index(t, (size_t) i) != 0;
        get_errors += expand_local_reference(t) != i;
        delete_errors += delete_reference(t) != 0;
        delete_errors += expand_local_reference(t) != REF_NOT_FOUND;
    }
    CU_ASSERT_EQUAL(hash_table_size, old_size);
    CU_ASSERT_EQUAL(insert_errors, 0);
    CU_ASSERT_EQUAL(get_errors, 0);
    CU_ASSERT_EQUAL(delete_errors, 0);

    get_errors = 0;
    for (int i = 0 ; i < 1000 ; ++i) {
        reference_tag t = {.raw_val = 0xbeef00000000 + i};
        get_errors = expand_local_reference(t) != (size_t) i;
    }
    CU_ASSERT_EQUAL(get_errors, 0);
}

void
t_grow_hash_table(void)
{
    extern size_t hash_table_size;
    size_t old_size = hash_table_size;

    int insert_errors = 0;
    for (size_t i = 0 ; i < old_size * 2 ; ++i) {
        reference_tag t = {.raw_val = 0xbabe00000000 + i};
        insert_errors += compress_absolute_index(t, (size_t) i) != 0;
    }
    CU_ASSERT_EQUAL(insert_errors, 0);
    CU_ASSERT_NOT_EQUAL(old_size, hash_table_size);

    int get_errors = 0;
    for (size_t i = 0 ; i < old_size * 2 ; ++i) {
        reference_tag t = {.raw_val = 0xbabe00000000 + i};
        get_errors += expand_local_reference(t) != i;
    }
    CU_ASSERT_EQUAL(get_errors, 0);
}

void
t_delete_all_for_pool(void)
{
    CU_ASSERT_NOT_EQUAL(delete_all_for_pool(0), 0);
    CU_ASSERT_EQUAL(delete_all_for_pool(0xbabe00000000), 0);

    int delete_errors = 0;
    for (size_t i = 0 ; i < PAGE_SIZE * 2 ; ++i) {
        reference_tag t = {.raw_val = 0xbabe00000000 + i};
        delete_errors += expand_local_reference(t) != REF_NOT_FOUND;
    }
    CU_ASSERT_EQUAL(delete_errors, 0);
}

