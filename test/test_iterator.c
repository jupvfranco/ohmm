#include "test_iterator.h"
#include <stdio.h>

/* 
 * These test-functions assume that a type table has been initialized with the
 * following types:
 *
 * Type 0: 1 byte char
 * Type 1: 8 byte double or 64 bit int 
 * Type 2: 8 byte reference
 * Type 3: Composite type, char + char + char + double
 * Type 4: Composite type, Double + Type 3 + Type 3
 * Type 5: A global reference to a list type
 * Type 6: A local reference to a list type
 *
 */
#include "basic_types.h"

void
t_pool_get_ref(void)
{
    pool_reference pool_0 = pool_create(LONG_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(pool_0, NULL_POOL);

    int reference_errors = 0;
    int value_errors = 0;
    for (long long i = 0 ; i < 1000 ; ++i) {
        global_reference x = pool_alloc(&pool_0);
        global_reference y = pool_get_ref(pool_0, i);
        set_field(x, 0, &i);
        reference_errors += x != y;
        long long *y_val = get_field(y, 0);
        value_errors += *y_val != i;
    }

    CU_ASSERT_EQUAL(reference_errors, 0);
    CU_ASSERT_EQUAL(value_errors, 0);
    pool_destroy(&pool_0);
}

void
t_iterator_simple_next_and_prev(void)
{
    pool_reference pool_0 = pool_create(LONG_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(pool_0, NULL_POOL);

    pool_grow(&pool_0, 4096);

    pool_iterator it = iterator_from_pool(pool_0);
    CU_ASSERT_EQUAL(iterator_prev(it), ITERATOR_END);

    for (long long i = 0 ; i < 4095 ; ++i) {
        iterator_set_field(it, 0, &i);
        it = iterator_next(pool_0, it);
    }

    CU_ASSERT_EQUAL(iterator_next(pool_0, it), ITERATOR_END);

    int error_count = 0;
    for (long long i = 4094 ; i >= 0 ; --i) {
        it = iterator_prev(it);
        long long *it_val = iterator_get_field(it, 0);
        error_count += *it_val != i;
    }

    CU_ASSERT_EQUAL(error_count, 0);

    pool_destroy(&pool_0);
}

void
t_iterator_list_next(void)
{
    pool_reference list_pool = pool_create(LIST_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(list_pool, NULL_POOL);

    global_reference head = NULL_REF;

    for (int i = 0 ; i < 10000 ; ++i ) {
        uint64_t x = 0xdeadbeef00000000 + i;
        uint64_t y = 0xbabeface00000000 + i;
        global_reference tmp = pool_alloc(&list_pool);
        set_field(tmp, 1, &x);
        set_field(tmp, 2, &y);
        set_field_reference(tmp, 0, head);
        head = tmp;
    }

    pool_iterator itr = iterator_from_reference(head);
    int get_errors = 0;
    for (int i = 9999 ; i >= 0 ; --i) {
        uint64_t x = 0xdeadbeef00000000 + i;
        uint64_t y = 0xbabeface00000000 + i;
        uint64_t *val_x = iterator_get_field(itr, 1);
        uint64_t *val_y = iterator_get_field(itr, 2);
        get_errors += *val_x != x;
        get_errors += *val_y != y;
        itr = iterator_next(list_pool, itr);
    }
    CU_ASSERT_EQUAL(get_errors, 0);
    CU_ASSERT_EQUAL(itr, 0);

    pool_destroy(&list_pool);
}

void
t_iterator_list_insert(void)
{
    pool_reference list_pool = pool_create(LIST_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(list_pool, NULL_POOL);

    global_reference head = pool_alloc(&list_pool);
    pool_iterator itr = iterator_from_reference(head);

    int insert_errors = 0;
    for (int i = 0 ; i < 2000 ; i += 2) {
        uint64_t x = 0xdeadbeef00000000 + i;
        uint64_t y = 0xbabeface00000000 + i;
        iterator_set_field(itr, 1, &x);
        iterator_set_field(itr, 2, &y);
        insert_errors += iterator_list_insert(itr, pool_alloc(&list_pool));
        itr = iterator_next(list_pool, itr);
    }
    CU_ASSERT_EQUAL(insert_errors, 0);

    itr = iterator_from_reference(head);
    int get_errors = 0;
    for (int i = 0 ; i < 2000 ; i += 2) {
        uint64_t x = 0xdeadbeef00000000 + i;
        uint64_t y = 0xbabeface00000000 + i;
        uint64_t *val_x = iterator_get_field(itr, 1);
        uint64_t *val_y = iterator_get_field(itr, 2);
        get_errors += *val_x != x;
        get_errors += *val_y != y;
        itr = iterator_next(list_pool, itr);
    }
    CU_ASSERT_EQUAL(get_errors, 0);

    itr = iterator_from_reference(head);
    insert_errors = 0;
    for (int i = 1 ; i < 2000 ; i += 2) {
        uint64_t x = 0xdeadbeef00000000 + i;
        uint64_t y = 0xbabeface00000000 + i;
        insert_errors += iterator_list_insert(itr, pool_alloc(&list_pool));
        itr = iterator_next(list_pool, itr);
        iterator_set_field(itr, 1, &x);
        iterator_set_field(itr, 2, &y);
        itr = iterator_next(list_pool, itr);
    }
    CU_ASSERT_EQUAL(insert_errors, 0);

    itr = iterator_from_reference(head);
    get_errors = 0;
    for (int i = 0 ; i < 2000 ; ++i) {
        uint64_t x = 0xdeadbeef00000000 + i;
        uint64_t y = 0xbabeface00000000 + i;
        uint64_t *val_x = iterator_get_field(itr, 1);
        uint64_t *val_y = iterator_get_field(itr, 2);
        get_errors += *val_x != x;
        get_errors += *val_y != y;
        itr = iterator_next(list_pool, itr);
    }
    CU_ASSERT_EQUAL(get_errors, 0);

    pool_destroy(&list_pool);
}

void
t_iterator_list_remove(void)
{
    pool_reference list_pool = pool_create(LIST_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(list_pool, NULL_POOL);

    global_reference head = pool_alloc(&list_pool);
    pool_iterator itr = iterator_from_reference(head);

    int insert_errors = 0;
    for (int i = 0 ; i < 2000 ; ++i) {
        uint64_t x = 0xdeadbeef00000000 + i;
        uint64_t y = 0xbabeface00000000 + i;
        iterator_set_field(itr, 1, &x);
        iterator_set_field(itr, 2, &y);
        insert_errors += iterator_list_insert(itr, pool_alloc(&list_pool));
        itr = iterator_next(0, itr);
    }
    CU_ASSERT_EQUAL(insert_errors, 0);

    itr = iterator_from_reference(head);
    int delete_errors = 0;
    for (int i = 0 ; i < 1000 ; ++i) {
        delete_errors += iterator_list_remove(itr);
        itr = iterator_next(0, itr);
    }
    CU_ASSERT_EQUAL(delete_errors, 0);

    itr = iterator_from_reference(head);
    int get_errors = 0;
    for (int i = 0 ; i < 2000 ; i+=2) {
        uint64_t x = 0xdeadbeef00000000 + i;
        uint64_t y = 0xbabeface00000000 + i;
        uint64_t *val_x = iterator_get_field(itr, 1);
        uint64_t *val_y = iterator_get_field(itr, 2);
        get_errors += *val_x != x;
        get_errors += *val_y != y;
        itr = iterator_next(0, itr);
    }
    CU_ASSERT_EQUAL(get_errors, 0);

    pool_destroy(&list_pool);
}

static void
insert(pool_reference *pool, global_reference root, uint64_t value)
{
   uint64_t *val = get_field(root, 2);

   unsigned int side = *val > value ? 0 : 1;
   global_reference node = get_field_reference(root, side);

   if (NULL_REF == node) {
       global_reference new = pool_alloc(pool);
       set_field(new, 2, &value);
       set_field_reference(root, side, new); 
       return;
   }

   insert(pool, node, value);
}

static void
oct_insert(pool_reference *pool, global_reference root, uint64_t n, uint64_t *v)
{

    uint64_t *val = get_field(root, 9);
    *val = *v;
    *v = *v + 1;

    if (n == 0)
        return;

    for (int i = 0 ; i < 8 ; ++i ) {
        global_reference child = pool_alloc(pool);
        set_field_reference(root, i, child);
        oct_insert(pool, child, n -1, v);
    }
}


static int 
intcmp(const void *xp, const void *yp)
{
    const uint64_t x = *((const uint64_t *) xp);
    const uint64_t y = *((const uint64_t *) yp);
    if (x == y)
        return 0;

    return x < y ? -1 : 1;
}

void
t_iterator_btree(void)
{
    size_t tree_size = 10000;

    pool_reference btree_pool = pool_create(BTREE_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(btree_pool, NULL_POOL);

    global_reference root = pool_alloc(&btree_pool);

    srandom(0xdeadbeef);
    uint64_t expected[tree_size];
    expected[0] = random();
    set_field(root, 2, expected);

    for (size_t i = 1 ; i < tree_size ; ++i) {
        expected[i] = random();
        insert(&btree_pool, root, expected[i]);
    }

    qsort(expected, tree_size, sizeof(uint64_t), intcmp);

    pool_iterator itr = iterator_new(&btree_pool, &root);

    int next_fail_count = 0;
    int cmp_fail_count = 0;
    for (size_t i = 0 ; i < tree_size ; ++i) {
        pool_iterator next_itr = iterator_next(0, itr);
        next_fail_count += next_itr == ITERATOR_END; 

        uint64_t *v = (uint64_t*) iterator_get_field(itr, 2);
        cmp_fail_count += expected[i] != *v;
    }

    CU_ASSERT_EQUAL(next_fail_count, 0);
    CU_ASSERT_EQUAL(cmp_fail_count, 0);
    CU_ASSERT_EQUAL(iterator_next(0, itr), ITERATOR_END);

    iterator_destroy(&itr);
    pool_destroy(&btree_pool);
}

void
t_iterator_ntree(void)
{
    pool_reference otree_pool = pool_create(OTREE_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(otree_pool, NULL_POOL);

    global_reference root = pool_alloc(&otree_pool);
    uint64_t end_value = 0;
    oct_insert(&otree_pool, root, 4, &end_value);
    pool_iterator itr = iterator_new(&otree_pool, &root); 

    int next_fail_count = 0;
    int cmp_fail_count = 0;
    for (uint64_t i = 0 ; i < end_value ; ++i) {
        pool_iterator next_itr = iterator_next(0, itr);
        next_fail_count += next_itr == ITERATOR_END;
        uint64_t *v = iterator_get_field(itr, 9);
        cmp_fail_count += *v != i;
    }

    CU_ASSERT_EQUAL(next_fail_count, 0);
    CU_ASSERT_EQUAL(cmp_fail_count, 0);
    CU_ASSERT_EQUAL(iterator_next(0, itr), ITERATOR_END);

    iterator_destroy(&itr);
    pool_destroy(&otree_pool);
}
