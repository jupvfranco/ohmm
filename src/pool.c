/**
 * @brief Defines functions for pooled allocations
 *
 * @file pool.c
 * @author Martin Hagelin
 * @date October, 2014
 *
 */

#include <assert.h>
#include "pool.h"
#include "type_info.h"
#include "field_info.h"
#include "reference_table.h"
#include "pool_private.h"

Type_table type_table;
static volatile uint16_t NEXT_FREE_POOL_IDX = 1u;

pool_reference
pool_create(uint16_t type_id)
{
    size_t   pool_size  = type_table[type_id].type_size * PAGE_SIZE;
    uint16_t pool_idx   = NEXT_FREE_POOL_IDX;

    bool write_success;
    do {
        write_success = __atomic_compare_exchange_n(
                &NEXT_FREE_POOL_IDX,
                &pool_idx,
                pool_idx + 1,
                false,
                __ATOMIC_RELAXED,
                __ATOMIC_RELAXED);

    } while (! write_success );

    void *addr_hint     = (void*) POOL_IDX_TO_ADDR(pool_idx);
    void *mapped_addr   = mmap( addr_hint,
                                pool_size,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED,
                                0, 0);

    if (addr_hint != mapped_addr)
        return NULL_POOL;

    struct pool_reference ref = {.type_id       = type_id,
                                 .pool_id       = pool_idx,
                                 .sub_pool_id   = 0,
                                 .raw_index     = 0 };

    return ref.raw_val;
}

int
pool_destroy(pool_reference *pool)
{
    /* In future versions it would be advantageous to keep track of unused
     * pools, perferably through a bit map. So they can be easily reused.
     * This could be done as a part of garbage collection, and thanks to layout
     * of the system, "compactation" of heaps could be made with mremap, thus
     * requireing no copying or memory movement.
     *
     * For now the pages are just released however.
     */

    struct pool_reference *ref = (struct pool_reference*) pool;
    size_t sub_pool_size = type_table[ref->type_id].type_size * PAGE_SIZE;


    uintptr_t pool_start = POOL_IDX_TO_ADDR(ref->pool_id);
    size_t    pool_size  = (1 + ref->sub_pool_id)*sub_pool_size;

    if (0 != munmap((void*) pool_start, pool_size))
        return errno;

    *pool = NULL_POOL;

    return 0;
}

static global_reference
pool_add_elements(pool_reference *pool,const size_t num_elements)
{
    struct pool_reference *p_ref = (struct pool_reference*) pool;
    size_t type_size = type_table[p_ref->type_id].type_size;


    uint16_t space_left_in_pool = p_ref->full ? 0 : PAGE_SIZE - p_ref->index;
    if (num_elements <= space_left_in_pool) {
        /* Simple case, fits in allocated space */
        struct global_reference g_ref = {.raw_val = p_ref->raw_val};
        p_ref->index += num_elements;
	    p_ref->full = p_ref->index == 0 ? 1 : 0;
        return g_ref.raw_val;
    }

    uintptr_t new_pool_addr =   POOL_IDX_TO_ADDR(p_ref->pool_id) +
                                (p_ref->sub_pool_id + 1)*PAGE_SIZE*type_size;

    size_t elements_needed = num_elements - space_left_in_pool;
    size_t sub_pools_needed = SUB_POOLS_NEEDED(elements_needed);
    size_t new_pages_needed = sub_pools_needed*type_size;

    void *mapped_addr = mmap((void*) new_pool_addr,
                             new_pages_needed*PAGE_SIZE,
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED,
                             0, 0);

    if (mapped_addr != (void*) new_pool_addr)
        return NULL_REF;

    struct global_reference g_ref = {.raw_val = p_ref->raw_val };
    g_ref.reserved = 0;
    g_ref.sub_pool_id += p_ref->full;
    p_ref->sub_pool_id += sub_pools_needed;
    p_ref->index = elements_needed % PAGE_SIZE;
    p_ref->full = ! p_ref->index;

    return g_ref.raw_val;
}


global_reference
pool_alloc(pool_reference *pool)
{
    /* Some optimizations could be made if this is the common case */
    return pool_add_elements(pool, 1);
}

int
pool_grow(pool_reference *pool, const size_t num_elements)
{
    return pool_add_elements(pool, num_elements) == NULL_REF ? -1 : 0;
}

int
pool_shrink(pool_reference *pool, const size_t num_elements)
{
    struct pool_reference *p_ref = (struct pool_reference*) pool;

    if (num_elements < p_ref->full*PAGE_SIZE + p_ref->index) {
        /* Simple case, can't deallocate sub-pool */
        p_ref->index -= num_elements;
        p_ref->full = 0;
        return 0;
    }

    size_t s_pool_size = type_table[p_ref->type_id].type_size*PAGE_SIZE;
    size_t s_pools_to_remove = 1 + ((num_elements -1) / PAGE_SIZE);

    /*The last subpool is never removed*/
    if (p_ref->sub_pool_id < s_pools_to_remove) {
        s_pools_to_remove = p_ref->sub_pool_id;
    }

    size_t index_change = num_elements - s_pools_to_remove*PAGE_SIZE;
    size_t remove_size = s_pool_size*s_pools_to_remove;
    size_t new_sub_pool_id = p_ref->sub_pool_id - s_pools_to_remove;


    uintptr_t pool_start_addr = POOL_IDX_TO_ADDR(p_ref->pool_id);
    uintptr_t unmap_addr =  pool_start_addr + (new_sub_pool_id+1)*s_pool_size;

    if (0 != munmap((void*)unmap_addr, remove_size))
        return errno;

    p_ref->sub_pool_id = new_sub_pool_id;
    p_ref->index -= index_change;
    if (p_ref->index == 0 && index_change < PAGE_SIZE)
        p_ref->full = 1;

    return 0;
}

void*
get_field(const global_reference reference, const size_t field_nr)
{
    struct global_reference ref = {.raw_val = reference };
    assert(ref.is_extended == 0);

    return (void*) (GET_POOL_ADDR(ref) +
                    GET_SUB_POOL_SIZE(ref)*ref.sub_pool_id +
                    GET_FIELD_OFFSET(ref, field_nr) +
                    GET_FIELD_SIZE(ref, field_nr)*ref.index);
}

int
set_field(const global_reference reference,
          const size_t field_nr,
          const void* data)
{
    struct global_reference ref = {.raw_val = reference };

    size_t field_size = GET_FIELD_SIZE(ref, field_nr);



    void *f_ptr = (void*) ( GET_POOL_ADDR(ref) +
                            GET_SUB_POOL_SIZE(ref)*ref.sub_pool_id +
                            GET_FIELD_OFFSET(ref, field_nr) +
                            ref.index*field_size );


    switch (field_size) {
        case 1: *((char*)f_ptr) = *((const char*)data);
                break;
        case 2: *((uint16_t*)f_ptr) = *((const uint16_t*)data);
                break;
        case 4: *((uint32_t*)f_ptr) = *((const uint32_t*)data);
                break;
        case 8: *((uint64_t*)f_ptr) = *((const uint64_t*)data);
                break;

        default:
                memcpy(f_ptr, data, field_size);
                break;
    }


    return 0;
}

int
set_field_reference(const global_reference this_ref,
                    const size_t field_nr,
                    const global_reference that_ref)
{

    reference_struct this = {.raw_val = this_ref};
    reference_struct that = {.raw_val = that_ref};

    uint16_t *that_local_ref_ptr = ((uint16_t*)
                                   (GET_POOL_ADDR(this) +
                                    GET_SUB_POOL_SIZE(this)*this.sub_pool_id +
                                    GET_FIELD_OFFSET(this,field_nr))) +
                                    this.index;

    if (that_ref == NULL_REF) {
        *that_local_ref_ptr = 0;
        return 0;
    }

    assert(this.pool_id == that.pool_id);

    size_t this_index = GET_GLOBAL_INDEX_OF_REF(this);
    size_t that_index = GET_GLOBAL_INDEX_OF_REF(that);
    int64_t difference = that_index - this_index;
    local_reference_struct old_ref = {.raw_val = *that_local_ref_ptr};


    if (difference >= (int)PAGE_SIZE || difference <= -1*(int)PAGE_SIZE) {
        reference_tag tag = { .raw_val = this.raw_val };
        local_reference_struct loc_ref = {  .index = field_nr,
                                            .is_long_ref = 1 };
        tag.local_ref = loc_ref.raw_val;

        if (0 != compress_absolute_index(tag, that_index))
            return 1;

        *that_local_ref_ptr = loc_ref.raw_val;
    } else if (old_ref.is_long_ref) {
        /*
         * If a long reference is replaced with a short reference, then the old
         * reference has to be explicitly deleted
         */
        reference_tag old_tag = {.raw_val = this_ref};
        old_tag.local_ref = old_ref.raw_val;
        delete_reference(old_tag);
    } else {
        local_reference_struct loc_ref = { .index = difference };
        *that_local_ref_ptr = loc_ref.raw_val;
    }

	return 0;
}

global_reference
get_field_reference(const global_reference this_ref,
                    const size_t field_nr)
{
    reference_struct this = {.raw_val = this_ref};
    uint16_t *that_local_ref_ptr = ((uint16_t*)
                                    (GET_POOL_ADDR(this) +
                                     GET_SUB_POOL_SIZE(this)*this.sub_pool_id +
                                     GET_FIELD_OFFSET(this, field_nr))) +
                                     this.index;

    if (0 == *that_local_ref_ptr)
        return NULL_REF;

    local_reference_struct that_local_ref = {.raw_val = *that_local_ref_ptr};

    size_t that_index;
    if (that_local_ref.is_long_ref) {
        reference_tag tag = { .raw_val = this.raw_val };
        tag.local_ref = that_local_ref.raw_val;
        that_index = expand_local_reference(tag);

        if (that_index == REF_NOT_FOUND)
            return NULL_REF;
    } else {

        size_t this_index = GET_GLOBAL_INDEX_OF_REF(this);
        that_index = this_index + that_local_ref.index;
    }

    reference_struct that = {.raw_val = this_ref};
    that.sub_pool_id = GLOBAL_INDEX_TO_SUBPOOL_ID(that_index);
    that.index = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(that_index);

    return that.raw_val;
}


void*
pool_to_array(const pool_reference pool)
{
    struct pool_reference p_ref = {.raw_val = pool};
    return (void*) POOL_IDX_TO_ADDR(p_ref.pool_id);
}
