/**
 * @brief Definitions for a basic map framework for pools.
 *
 * @file pool_map.c
 * @author Martin Hagelin
 * @date January 2015
 *
 */
#include <omp.h>

#include "basic_types.h"
#include "pool_private.h"
#include "reference_table.h"
#include "field_info.h"
#include "type_info.h"
#include "pool_map.h"

extern Type_table type_table;

int
field_map(const pool_reference A,
          pool_reference *B,
          size_t field_no,
          map_function_type f)
{
    pool_struct src_pool = {.raw_val = A};
    pool_struct dst_pool = {.raw_val = *B};

    size_t pool_size = GET_SIZE_OF_POOL(src_pool);
    size_t full_subpools = pool_size / PAGE_SIZE;
    size_t remainder = pool_size % PAGE_SIZE;

    size_t field_size = GET_FIELD_SIZE(src_pool, field_no);
    size_t sub_pool_size = GET_SUB_POOL_SIZE(src_pool);
    size_t field_offset = GET_FIELD_OFFSET(src_pool, field_no);

    size_t target_field_size = GET_FIELD_SIZE(dst_pool, 0);
    size_t target_sub_pool_size = GET_SUB_POOL_SIZE(dst_pool);

    if (pool_grow(B, pool_size) != 0)
        return 1;

    char *a_base = (char *) GET_POOL_ADDR(src_pool) + field_offset;
    char *b_base = (char *) GET_POOL_ADDR(dst_pool);


//    #pragma omp parallel for
    for (size_t i = 0 ; i < full_subpools ; ++i) {
        char *a = a_base + i*sub_pool_size;
        char *b = b_base + i*target_sub_pool_size;
        for (size_t j = 0 ; j < PAGE_SIZE ; ++j) {
            f(a+j*field_size, b+j*target_field_size);
        }
    }

    for (size_t i = 0 ; i < remainder ; ++i) {
        char *a = a_base + full_subpools*sub_pool_size + i*field_size;
        char *b = b_base + full_subpools*target_sub_pool_size +
                  i*target_field_size;
        f(a, b);
    }

    return 0;
}

int
field_list_map(const global_reference A,
	           pool_reference *B,
	           size_t field_no,
	           map_function_type f)
{
    reference_struct src_ref = {.raw_val = A};
    pool_struct dst_pool = {.raw_val = *B};

    size_t sub_pool_size = GET_SUB_POOL_SIZE(src_ref);
    size_t field_size = GET_FIELD_SIZE(src_ref, field_no);
    size_t field_offset = GET_FIELD_OFFSET(src_ref, field_no);
    size_t target_field_size = GET_FIELD_SIZE(dst_pool, 0);

    uintptr_t a_pool_start = GET_POOL_ADDR(src_ref);
    uintptr_t a_field_start = a_pool_start + field_offset;
    char *b = (char *) GET_POOL_ADDR(dst_pool);

	size_t idx = GET_GLOBAL_INDEX_OF_REF(src_ref);

    while (idx != REF_END) {
        if (NULL_REF == pool_alloc(B))
            return 1;

        void *a = (void*) ( a_field_start +  
                  sub_pool_size*GLOBAL_INDEX_TO_SUBPOOL_ID(idx) +
                  field_size*GLOBAL_INDEX_TO_SUBPOOL_OFFSET(idx) );

        f(a, b);
                  

        uint16_t *next_loc_ref = 
            ((uint16_t*) (a_pool_start +
            sub_pool_size*GLOBAL_INDEX_TO_SUBPOOL_ID(idx) )) +
            GLOBAL_INDEX_TO_SUBPOOL_OFFSET(idx);

        local_reference_struct next = {.raw_val = *next_loc_ref};

        if (next.is_long_ref) {
            reference_tag t = {.raw_val = A};
            t.sub_pool_id = GLOBAL_INDEX_TO_SUBPOOL_ID(idx);
            t.index = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(idx);
            t.local_ref = next.raw_val;
            idx = expand_local_reference(t);
        } else if (next.index == 0) {
            idx = REF_END;
        } else {
            idx += next.index;
        }


        b += target_field_size;
    }
    
    return 0;
}
