
#include "test_pool_map.h"

static void 
square(void *x, void *y)
{
    *((uint64_t*) y) = (*((uint64_t*)x)) * (*((uint64_t*)x));
}

void
t_field_map(void)
{
    pool_reference list_pool = pool_create(LIST_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(list_pool, NULL_POOL);

    global_reference head = pool_alloc(&list_pool);
    pool_iterator itr = iterator_new(&list_pool, &head);

    size_t list_size = 10000;

    for (size_t i = 0 ; i < list_size ; ++i) {
        iterator_set_field(itr, 1, &i);
        iterator_list_insert(itr, pool_alloc(&list_pool));
        itr = iterator_next(list_pool, itr);
    }

    pool_reference long_pool = pool_create(LONG_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(long_pool, NULL_POOL);

    CU_ASSERT_EQUAL(field_map(list_pool, &long_pool, 1, square), 0);

    uint64_t *result = pool_to_array(long_pool);

    int cmp_error_count = 0;
    for (size_t i = 0 ; i < list_size ; ++i) {
        cmp_error_count += i*i != result[i];
    }

    CU_ASSERT_EQUAL(cmp_error_count, 0);

    iterator_destroy(&itr);
    pool_destroy(&long_pool);
    pool_destroy(&list_pool);
}

void
t_field_list_map(void)
{
    pool_reference list_pool = pool_create(LIST_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(list_pool, NULL_POOL);

    global_reference head = pool_alloc(&list_pool);
    pool_iterator itr = iterator_new(&list_pool, &head);

    size_t list_size = 10000;

    for (size_t i = 0 ; i < list_size ; ++i) {
        iterator_set_field(itr, 1, &i);
        iterator_list_insert(itr, pool_alloc(&list_pool));
        itr = iterator_next(list_pool, itr);
        pool_alloc(&list_pool); /* "Deleted" element */
    }

    pool_reference long_pool = pool_create(LONG_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(long_pool, NULL_POOL);

    CU_ASSERT_EQUAL(field_list_map(head, &long_pool, 1, square), 0);
    uint64_t *result = pool_to_array(long_pool);

    int cmp_error_count = 0;
    for (size_t i = 0 ; i < list_size ; ++i) {
        cmp_error_count += i*i != result[i];
    }

    CU_ASSERT_EQUAL(cmp_error_count, 0);

    iterator_destroy(&itr);
    pool_destroy(&long_pool);
    pool_destroy(&list_pool);

}
