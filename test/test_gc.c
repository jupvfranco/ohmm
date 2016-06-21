#include <stdlib.h>

#include "test_gc.h"
#include "pool_private.h"

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
t_collect_list_pool(void)
{
    pool_reference list_pool = pool_create(LIST_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(list_pool, NULL_POOL);
    
    global_reference head = pool_alloc(&list_pool);
    pool_iterator itr = iterator_from_reference(head);

    int insert_errors = 0;
    for (int i = 0 ; i < 8000 ; ++i) {
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
    for (int i = 0 ; i < 4000 ; ++i) {
        delete_errors += iterator_list_remove(itr);
        itr = iterator_next(0, itr);
    }
    CU_ASSERT_EQUAL(delete_errors, 0);

    CU_ASSERT_EQUAL(gc_init(), 0);
    CU_ASSERT_EQUAL(push_root(&head), 0);
    int res = collect_pool(&list_pool);
    pool_struct pool = {.raw_val = list_pool};

    CU_ASSERT_EQUAL(res, 0);
    CU_ASSERT_NOT_EQUAL(list_pool, NULL_POOL);
    CU_ASSERT_EQUAL(pool.index, 4001); /* 4000 elements + the head */

    itr = iterator_from_reference(head);
    int get_errors = 0;
    for (int i = 0 ; i < 8000 ; i+=2) {
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

   unsigned int side = *val < value ? 0 : 1;
   global_reference node = get_field_reference(root, side);

   if (NULL_REF == node) {
       global_reference new = pool_alloc(pool);
       set_field(new, 2, &value);
       set_field_reference(root, side, new); 
       return;
   }

   insert(pool, node, value);
}

void
t_collect_btree_pool(void)
{
    pool_reference btree_pool = pool_create(BTREE_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(btree_pool, NULL_POOL);

    srandom(0xdeadbeef);

    global_reference root_1 = pool_alloc(&btree_pool);
    global_reference root_2 = pool_alloc(&btree_pool);

    uint64_t v = random();
    set_field(root_1, 2, &v);
    v = random();
    set_field(root_2, 2, &v);

    for (int i = 0 ; i < 10000 ; ++i) {
        insert(&btree_pool, root_1, random());
        insert(&btree_pool, root_2, random());
        pool_alloc(&btree_pool);
    }

    pool_struct pool = {.raw_val = btree_pool};
    CU_ASSERT_EQUAL(pool.index + PAGE_SIZE*pool.sub_pool_id, 30002);

    push_root(&root_1);
    push_root(&root_2);
    CU_ASSERT_EQUAL(collect_pool(&btree_pool), 0);

    pool.raw_val = btree_pool;
    CU_ASSERT_EQUAL(pool.index + PAGE_SIZE*pool.sub_pool_id, 20002);

    /* TODO Iterate over trees and verify contents */

    pool_destroy(&btree_pool);
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
        pool_alloc(pool);  /* Junk element */
        set_field_reference(root, i, child);
        oct_insert(pool, child, n -1, v);
    }
}

void
t_collect_ntree_pool(void)
{
    pool_reference otree_pool = pool_create(OTREE_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(otree_pool, NULL_POOL);

    global_reference root = pool_alloc(&otree_pool);
    uint64_t end_value = 0;
    oct_insert(&otree_pool, root, 4, &end_value);

    pool_struct p = {.raw_val = otree_pool };
    CU_ASSERT_EQUAL(p.index + PAGE_SIZE*p.sub_pool_id, 2*end_value - 1);

    push_root(&root);
    CU_ASSERT_EQUAL(collect_pool(&otree_pool), 0);

    p.raw_val = otree_pool;
    CU_ASSERT_EQUAL(p.index + PAGE_SIZE*p.sub_pool_id, end_value);

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

