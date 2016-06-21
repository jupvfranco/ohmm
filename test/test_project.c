#include <stdio.h>
#include "CUnit/Basic.h"

#include "test_type_info.h"
#include "test_pool.h"
#include "test_iterator.h"
#include "test_reference_table.h"
#include "test_gc.h"
#include "test_pool_map.h"

#define DIE(msg) do { fprintf(stderr, "Failed to add test %s\n", msg) ; goto cleanup;} while (0)

const char const * const type_info_names[] = {
    "get_size_and_field_count",
    "fill_in_offsets",
    "init_type_table"
};

void (* const type_info_tests[]) (void) = {
    t_get_size_and_field_count,
    t_fill_in_offsets,
    t_init_type_table
};

const char const * const pool_names[] = {
    "pool_create",
    "pool_alloc",
    "set_field",
    "get_field", 
    "pool_grow",
    "pool_shrink",
    "pool_destroy",
    "(set|get)_field_reference"
};

void (* const pool_tests[]) (void) = {
    t_pool_create,
    t_pool_alloc,
    t_set_field,
    t_get_field,
    t_pool_grow,
    t_pool_shrink,
    t_pool_destroy,
    t_set_and_get_field_reference
};

const char const * const iterator_names[] = {
    "pool_get_ref",
    "iterator_simple_(next|prev)",
    "iterator_list_next",
    "iterator_list_insert",
    "iterator_list_remove",
    "iterator_btree",
    "iterator_ntree"
};

void (* const iterator_tests[]) (void) = {
    t_pool_get_ref,
    t_iterator_simple_next_and_prev,
    t_iterator_list_next,
    t_iterator_list_insert,
    t_iterator_list_remove,
    t_iterator_btree,
    t_iterator_ntree
};

const char const * const map_names[] = {
    "t_field_map",
    "t_field_list_map"
};

void (* const map_tests[]) (void) = {
    t_field_map,
    t_field_list_map
};

const char const * const reference_table_names[] = {
    "expand_and_compress_local_reference",
    "delete_reference",
    "cleanup_hash_table",
    "grow_hash_table",
    "delete_all_for_pool"
};

void (* const reference_table_tests[]) (void) = {
    t_expand_and_compress_local_reference,
    t_delete_reference,
    t_cleanup_hash_table,
    t_grow_hash_table,
    t_delete_all_for_pool
};

const char const * const gc_names[] = {
    "t_collect_list_pool",
    "t_collect_btree_pool",
    "t_collect_ntree_pool"
};

void (* const gc_tests[]) (void) = {
    t_collect_list_pool,
    t_collect_btree_pool,
    t_collect_ntree_pool
};

int
main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    CU_pSuite type_info_suite = CU_add_suite("Type Info Suite", NULL, NULL);
    if (NULL == type_info_suite)
        DIE("suite for type info");

    CU_pSuite pool_suite = CU_add_suite("Pool Suite", NULL, NULL);
    if (NULL == pool_suite)
        DIE("suite for pool allocator");

    CU_pSuite iterator_suite = CU_add_suite("Iterator Suite", NULL, NULL);
    if (NULL == iterator_suite)
        DIE("suite for iterator");

    CU_pSuite map_suite = CU_add_suite("Map Suite", NULL, NULL);
    if (NULL == map_suite)
        DIE("suite for maps");

    CU_pSuite ref_table_suite = CU_add_suite("Reference Table Suite", NULL, NULL);
    if (NULL == ref_table_suite)
        DIE("suite for reference table");

    CU_pSuite gc_suite = CU_add_suite("Garbage Collection Suite", NULL, NULL);
    if (NULL == gc_suite)
        DIE("suite for garbage collection");

    for (unsigned i = 0 ; i < sizeof(type_info_names) / sizeof(void*) ; ++i)
        if (NULL == CU_add_test(type_info_suite,
                                type_info_names[i],
                                type_info_tests[i] ))
            DIE(type_info_names[i]);

    for (unsigned i = 0 ; i < sizeof(pool_names) / sizeof(void*) ; ++i)
        if (NULL == CU_add_test(pool_suite,
                                pool_names[i],
                                pool_tests[i] ))
            DIE(pool_names[i]);

    for (unsigned i = 0 ; i < sizeof(iterator_names) / sizeof(void*) ; ++i)
        if (NULL == CU_add_test(iterator_suite,
                                iterator_names[i],
                                iterator_tests[i] ))
            DIE(iterator_names[i]);

    for (unsigned i = 0 ; i < sizeof(reference_table_names) / sizeof(void*); ++i)
        if (NULL == CU_add_test(ref_table_suite,
                                reference_table_names[i],
                                reference_table_tests[i] ))
            DIE(reference_table_names[i]);

    for (unsigned i = 0 ; i < sizeof(map_names) / sizeof(void*) ; ++i)
        if (NULL == CU_add_test(map_suite,
                                map_names[i],
                                map_tests[i]))
            DIE(map_names[i]);

    for (unsigned i = 0 ; i < sizeof(gc_names) / sizeof(void*); ++i)
        if (NULL == CU_add_test(gc_suite,
                                gc_names[i],
                                gc_tests[i]))
            DIE(gc_names[i]);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

cleanup:

    CU_cleanup_registry();
    return CU_get_error();
}
