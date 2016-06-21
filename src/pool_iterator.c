/**
 * @brief Defines functions for iterators of composite objects in pools.
 *
 * @file pool_iterator.c
 * @author Martin Hagelin
 * @date December 2014
 *
 */

#include <assert.h>

#include "basic_types.h"
#include "pool_iterator.h"
#include "pool_private.h"
#include "reference_table.h"
#include "field_info.h"
#include "type_info.h"

extern Type_table type_table;

/* Gets the number of local references a type has */
static inline size_t
get_reference_count(uint16_t type_id);

/* Common parts for creation of different iterators */
static inline pool_iterator
iterator_common(struct pool_iterator itr);

/* In order walk of a binary tree */
static inline pool_iterator
iterator_btree_next(complex_iterator_struct *cis, pool_iterator iter);

/* Unspecifid  walk of n-ary tree */
static inline pool_iterator
iterator_ntree_next(complex_iterator_struct *cis, pool_iterator iter);

INLINED global_reference
pool_get_ref(pool_reference pool, size_t index)
{
    uint64_t sub_pool_id = GLOBAL_INDEX_TO_SUBPOOL_ID(index);
    uint16_t elem_index = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(index);

    /* TODO Support for larger pools */
    assert(!(sub_pool_id >> 16));


    reference_struct ref = {.raw_val = pool};
    ref.sub_pool_id = sub_pool_id;
    ref.raw_index = elem_index;

    return ref.raw_val;
}

INLINED pool_iterator
iterator_simple_next(pool_reference pool_ref, pool_iterator iterator)
{
    reference_struct ref = {.raw_val = iterator};
    pool_struct pool = {.raw_val = pool_ref };

    uint16_t index = ref.index + 1;
    uint16_t sub_pool_id = ref.sub_pool_id + GLOBAL_INDEX_TO_SUBPOOL_ID(index);

    if ((sub_pool_id < pool.sub_pool_id) || 
        (sub_pool_id == pool.sub_pool_id && index < pool.index) ||
        (pool.full && index < PAGE_SIZE)){

        ref.index = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(index);
        ref.sub_pool_id = sub_pool_id;
        return ref.raw_val;
    }

    return ITERATOR_END;
}

INLINED pool_iterator
iterator_list_next(pool_iterator iterator)
{
    iterator_struct itr = {.raw_val = iterator};


    /* Current invariant of lists: reference is first field -> easy field
     * offset */
    uint16_t *next_raw_val = ((uint16_t*)
                               (GET_POOL_ADDR(itr) +
                                GET_SUB_POOL_SIZE(itr)*itr.sub_pool_id)) +
                                itr.index;
    local_reference_struct next = {.raw_val = *next_raw_val};

    /* Not yet supported! */
    assert(! next.is_long_ref);

    if (0 == next.index)
        return ITERATOR_END;

    size_t global_index = GET_GLOBAL_INDEX_OF_REF(itr);
    global_index += next.index; 

    itr.sub_pool_id = GLOBAL_INDEX_TO_SUBPOOL_ID(global_index);
    itr.index = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(global_index);

    return itr.raw_val;
}


INLINED pool_iterator
iterator_tree_next(pool_iterator iterator)
{
    struct pool_iterator simple_iterator = {.raw_val = iterator};
    simple_iterator.iterator_type = 0;
    complex_iterator_struct *cis = (void*) simple_iterator.raw_val; 
    
    if (cis->num_children == 2)
        return iterator_btree_next(cis, iterator);
    else
        return iterator_ntree_next(cis, iterator);
}




pool_iterator
iterator_new(pool_reference *pool, global_reference *root)
{
    if (NULL == pool || *pool == NULL_REF) {
        if (NULL == root || *root == NULL_REF)
            return NULL_ITERATOR;

        return iterator_from_reference(*root);
    }

    if (NULL == root || *root == NULL_REF)
        return iterator_from_pool(*pool);

    pool_struct ps = {.raw_val = *pool};
    size_t ref_count = get_reference_count(ps.type_id);

    struct pool_iterator itr = {.raw_val = *root};
    if (ref_count == 0) {
        itr.iterator_type = ITERATOR_SIMPLE;
    } else if (ref_count == 1) {
        itr.iterator_type = ITERATOR_LIST;
    } else {
        pool_reference itr_pool = pool_create(LONG_TYPE_ID);
        if (NULL_REF == itr_pool)
            return NULL_ITERATOR;

        pool_grow(&itr_pool, 1 + sizeof(complex_iterator_struct) / 8);
        complex_iterator_struct *cis = pool_to_array(itr_pool);
        cis->root = root;
        cis->pool = pool;
        cis->iter_pool = itr_pool;
        cis->pool_start = (void*)GET_POOL_ADDR(ps);
        cis->num_children = ref_count;
        cis->elem_size = type_table[ps.type_id].type_size;

        cis->cursor = REF_BEGIN;
        cis->prev = REF_BEGIN;
        cis->next = GET_GLOBAL_INDEX_OF_REF(((reference_struct) 
                                            {.raw_val = *root}));

        itr.raw_val = (uint64_t) cis;
        assert(!itr.iterator_type);
        itr.iterator_type = ITERATOR_COMPLEX;
    }

    return itr.raw_val;
}

void
iterator_destroy(pool_iterator *iterator)
{
    struct pool_iterator itr = {.raw_val = *iterator};

    if (itr.iterator_type == ITERATOR_COMPLEX) {
        itr.iterator_type = 0;
        complex_iterator_struct *cis = (void*) itr.raw_val;
        pool_reference tmp = cis->iter_pool;
        pool_destroy(&tmp);
    }

    iterator = NULL;
}

pool_iterator
iterator_from_reference(global_reference ref)
{
    struct pool_iterator itr = {.raw_val = ref };

    return iterator_common(itr);
}

pool_iterator
iterator_from_pool(pool_reference pool)
{
    struct pool_iterator itr = {.raw_val = pool_get_ref(pool, 0) };
    return iterator_common(itr);
}

pool_iterator
iterator_next(pool_reference pool_ref, pool_iterator iterator)
{
    struct pool_iterator itr = {.raw_val = iterator};

    switch (itr.iterator_type) {
        case ITERATOR_SIMPLE: return iterator_simple_next(pool_ref, iterator);
        case ITERATOR_LIST: return iterator_list_next(iterator);
    case ITERATOR_LIST_COMPACT: return iterator_list_next(iterator);
        case ITERATOR_COMPLEX: return iterator_tree_next(iterator);
       /*
    * TODO Implement: In this case the iterator is really a slightly modified
    * pointer to complex_iterator_struct
    */
    }

    /* Won't reach this */
    return ITERATOR_END;
}

pool_iterator
iterator_prev(pool_iterator iterator)
{
    iterator_struct itr = {.raw_val = iterator};

    /* 
     * The operation is not supported for list-like structures (no double
     * linked lists yet)
     */
    assert(itr.iterator_type == ITERATOR_SIMPLE);

    if (itr.index > 0) {
        itr.index --;
        return itr.raw_val;
    }

    if (itr.sub_pool_id > 0) {
        itr.index = PAGE_SIZE - 1;
        itr.sub_pool_id --;
        return itr.raw_val;
    }

    return ITERATOR_END;
}

void*
iterator_get_field(pool_iterator iterator, const size_t field)
{
    iterator_struct itr = {.raw_val = iterator };

    if (itr.iterator_type != ITERATOR_COMPLEX)
        return get_field(iterator, field);

    itr.iterator_type = 0;
    complex_iterator_struct *cis = (void*) itr.raw_val;
    
    if (cis->cursor & REF_INVALID_BIT)
        return NULL;
    /* 
     * TODO Remove unnecessary work, perhaps by adding a get function that
     * takes an index as argument?
     */
    global_reference ref = pool_get_ref(*cis->pool, cis->cursor);
    return get_field(ref, field);
}

int
iterator_set_field(pool_iterator iterator,
                   const size_t field,
                   const void *data)
{
    iterator_struct itr = {.raw_val = iterator };

    if (itr.iterator_type != ITERATOR_COMPLEX)
        return set_field(iterator, field, data);

    itr.iterator_type = 0;
    complex_iterator_struct *cis = (void*) itr.raw_val;
    
    if (cis->cursor & REF_INVALID_BIT)
        return 1;
    /* 
     * TODO Remove unnecessary work, perhaps by adding a set function that
     * takes an index as argument?
     */
    global_reference ref = pool_get_ref(*cis->pool, cis->cursor);
    return set_field(ref, field, data);
}


int
iterator_list_insert(pool_iterator iterator, global_reference reference)
{
    iterator_struct itr = {.raw_val = iterator};
    assert(itr.iterator_type == ITERATOR_LIST);
    
    global_reference next = get_field_reference(iterator, 0);
    set_field_reference(reference, 0, next);
    set_field_reference(iterator, 0, reference);

    return 0;
}

int
iterator_list_remove(pool_iterator iterator)
{
    iterator_struct itr = {.raw_val = iterator};
    assert(itr.iterator_type == ITERATOR_LIST);

    global_reference next = get_field_reference(iterator, 0);
    if (NULL_REF == next)
        return 2;   /* TODO: Enum with return code values */

    global_reference next_next = get_field_reference(next, 0);

    return set_field_reference(iterator, 0, next_next);
}

static inline size_t
get_reference_count(uint16_t type_id)
{
    size_t i = 0;
    for ( ; i < type_table[type_id].field_count ; ++i) {
        uint16_t field_type_id = type_table[type_id].field_offsets[i].type_id;
        if (type_table[field_type_id].type_class != LOCAL_REF_TYPE)
            break;
    }
    return i;
}

static inline pool_iterator
iterator_common(struct pool_iterator itr)
{
    size_t self_reference_count = get_reference_count(itr.type_id);

    if (self_reference_count == 0) {
        itr.iterator_type = ITERATOR_SIMPLE;
    }
    else if (self_reference_count == 1) {
        itr.iterator_type = ITERATOR_LIST;
    }
    else {
    return NULL_ITERATOR;
    }


    return itr.raw_val;
}

static inline size_t
get_field_ref(complex_iterator_struct *cis, size_t elem, size_t field_no)
{
    uint16_t subpool = GLOBAL_INDEX_TO_SUBPOOL_ID(elem);
    uint16_t index = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(elem);

    uint16_t *loc_ref_ptr = ((uint16_t*)
                            (((char *)cis->pool_start) +
                            cis->elem_size*PAGE_SIZE * subpool)) +
                            PAGE_SIZE*field_no + index;

    local_reference_struct loc_ref = {.raw_val = *loc_ref_ptr};

    if (loc_ref.is_long_ref) {
        reference_tag t = { .raw_val = *cis->root };
        t.sub_pool_id = subpool;
        t.index = index;
        t.local_ref = loc_ref.raw_val;
        return expand_local_reference(t);
    }
    if (loc_ref.index == 0)
        return REF_END;

    return elem + loc_ref.index;
}

/* GCC refuses to inline these. */
static void
push(complex_iterator_struct *cis, size_t index)
{
    assert(pool_alloc(&cis->iter_pool));
    cis->stack[cis->n++] = index;
}

/* GCC refuses to inline these. */
static size_t
pop(complex_iterator_struct *cis)
{
    if (cis->n == 0)
        return REF_END;

    size_t index = cis->stack[--cis->n];
    pool_shrink(&cis->iter_pool, 1);
    return index;
}

static pool_iterator
iterator_btree_next(complex_iterator_struct *cis, pool_iterator iter)
{
    if (cis->next == REF_END && cis->n == 0)
        return ITERATOR_END;

    while (cis->next != REF_END) {
        push(cis, cis->next);
        cis->next = get_field_ref(cis, cis->next, 0);
    }

    cis->cursor = pop(cis);
    cis->next = get_field_ref(cis, cis->cursor, 1);

    return iter;
}

static pool_iterator
iterator_ntree_next(complex_iterator_struct *cis, pool_iterator iter)
{
    if (cis->next == REF_END)
        return ITERATOR_END;

    for (int i = cis->num_children - 1 ; i >= 0 ; --i) {
        size_t child = get_field_ref(cis, cis->next, i);

        if (child == REF_END)
            continue;

        push(cis, child);
    }

    cis->cursor = cis->next;
    cis->next = pop(cis);

    return iter;
}
