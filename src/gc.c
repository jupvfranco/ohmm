/**
 * @brief Defines functions used for collecting and compressing heaps.
 *
 * @file gc.h
 * @author Martin Hagelin
 * @date Decmber, 2014
 *
 */

#include "gc.h"
#include "pool_iterator.h"

/*
typedef enum gc_state_enum {

}
*/

#define ONE_STEP (((local_reference_struct) { .index = 1 }).raw_val)
#define OUT_OF_MEM (~((size_t)0u))

extern Type_table type_table;

static pool_reference root_stack_pool;
static size_t root_stack_size;
static global_reference **root_stack;


static global_reference*
pop_root(void);

/* Prototypes for helper functions */
static inline void
copy_data_fields(void *dst_spool,
                 void *src_spool,
                 size_t dst_idx,
                 size_t src_idx,
                 Field_offsets field_offsets,
                 size_t n);

static size_t
move_list(pool_reference *dst_pool,
          pool_reference *src_pool,
          size_t src_idx);

static global_reference
move_btree(pool_reference *dst_pool,
           pool_reference *src_pool,
           global_reference root);

static int
move_ntree(pool_reference *dst_pool,
           pool_reference *src_pool,
           global_reference root,
           global_reference new_root,
           size_t field_count,
           size_t ref_field_count);



/* Exported symbols */
int
gc_init(void)
{
    root_stack_pool = pool_create(LONG_TYPE_ID);
    if (NULL_POOL == root_stack_pool)
        return 1;

    root_stack = pool_to_array(root_stack_pool);
    return 0;
}

int
push_root(global_reference *root)
{
    if (NULL_REF == pool_alloc(&root_stack_pool))
        return 1;

    root_stack[root_stack_size++] = root;
    return 0;
}

int
collect_pool(pool_reference *pool)
{
    pool_struct src = { .raw_val = *pool };
    pool_reference dst = pool_create(src.type_id);

    if (NULL_POOL == dst)
        return 1;   /* TODO Better return codes */

    size_t field_count = type_table[src.type_id].field_count;
    size_t num_refs = 0;
    for (size_t i = 0 ; i < field_count ; ++i ) {
        uint16_t field_id = type_table[src.type_id].field_offsets[i].type_id;

        if (LOCAL_REF_TYPE != type_table[field_id].type_class) 
            break;

        num_refs++;
    }

    if (num_refs == 1) {
        while (root_stack_size > 0) {
            global_reference *root = pop_root();
            reference_struct root_ref = {.raw_val = *root};
            size_t src_idx = GET_GLOBAL_INDEX_OF_REF(root_ref);

            size_t dst_idx = move_list(&dst, pool, src_idx);

            if (dst_idx == OUT_OF_MEM) {
                pool_destroy(&dst);
                return 2;
                /* TODO Better error handling! 
                 * Might end up in a junk state if one root is changed
                 * already! */
            }

            reference_struct new_ref = {
                .pool_id = ((reference_struct) {.raw_val = dst}).pool_id,
                .sub_pool_id = GLOBAL_INDEX_TO_SUBPOOL_ID(dst_idx),
                .type_id = src.type_id,
                .index = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(dst_idx) };
            *root = new_ref.raw_val;
        }
    } else if (num_refs == 2) {
        while (root_stack_size > 0) {
            global_reference *root = pop_root();
            global_reference new_root = move_btree(&dst, pool, *root);
            if (new_root == NULL_REF) {
                pool_destroy(&dst);
                return 2;
            }
            *root = new_root;
        }
    } else {
        while (root_stack_size > 0) {
            global_reference *root = pop_root();
            global_reference new_root = pool_alloc(&dst);
            if (0 != move_ntree(&dst,
                                 pool,
                                *root,
                                 new_root,
                                 field_count,
                                 num_refs)) {
                pool_destroy(&dst);
                return 2;
            }
            *root = new_root;
        }
    }

    
    pool_destroy(pool);
    *pool = dst;

    return 0;
}



/* Helper functions */


static global_reference*
pop_root(void)
{
    global_reference *ret_val = root_stack[--root_stack_size];
    pool_shrink(&root_stack_pool, 1);
    return ret_val;
}

static inline void
copy_data_fields(void *dst_spool,
                 void *src_spool,
                 size_t dst_idx,
                 size_t src_idx,
                 Field_offsets field_offsets,
                 size_t n)
{
    for (size_t i = 0 ; i <  n ; ++i) {
        size_t field_size = field_offsets[i].field_size;
        void *dst = ((char*)dst_spool) + field_offsets[i].offset*PAGE_SIZE;
        void *src = ((char*)src_spool) + field_offsets[i].offset*PAGE_SIZE;

        switch(field_size) {
            case 1: ((char*)dst)[dst_idx] = ((char*)src)[src_idx];
                    break;
            case 2: ((uint16_t*)dst)[dst_idx] = ((uint16_t*)src)[src_idx];
                    break;
            case 4: ((uint32_t*)dst)[dst_idx] = ((uint32_t*)src)[src_idx];
                    break;
            case 8: ((uint64_t*)dst)[dst_idx] = ((uint64_t*)src)[src_idx];
                    break;

            default:
                    memcpy(((char*)dst) + field_size*dst_idx,
                           ((char*)src) + field_size*src_idx,
                           field_size);
                    break;
        }
    }
}

/* 
 * Using different functions since different layouts might be used for
 * trees as opposed to lists or general graphs.
 */
static size_t
move_list(pool_reference *dst_pool,
          pool_reference *src_pool,
          size_t src_idx)
{
    extern Type_table type_table;

    pool_struct src = { .raw_val = *src_pool};

    char *src_base = pool_to_array(*src_pool);
    char *dst_base = pool_to_array(*dst_pool);

    reference_struct head_ref = {.raw_val = pool_alloc(dst_pool)};
    if (head_ref.raw_val == NULL_REF)
        return OUT_OF_MEM;

    Field_offsets field_offsets = type_table[src.type_id].field_offsets;
    size_t field_count = type_table[src.type_id].field_count;
    size_t spool_size = GET_SUB_POOL_SIZE(src);
    size_t start_idx = GET_GLOBAL_INDEX_OF_REF(head_ref);

    size_t dst_idx = start_idx;

    while (src_idx != REF_NOT_FOUND) {

        char *src_spool = src_base + GLOBAL_INDEX_TO_SUBPOOL_ID(src_idx)*spool_size;
        char *dst_spool = dst_base + GLOBAL_INDEX_TO_SUBPOOL_ID(dst_idx)*spool_size;
        size_t src_sp_idx = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(src_idx);
        size_t dst_sp_idx = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(dst_idx);
        size_t next_idx = REF_NOT_FOUND;

        local_reference_struct next = {
            .raw_val = *((uint16_t*) src_spool + src_sp_idx) };

        if (next.is_long_ref) {
            reference_tag tag = {
                .local_ref = next.raw_val,
                .sub_pool_id = GLOBAL_INDEX_TO_SUBPOOL_ID(src_idx),
                .pool_id = src.pool_id,
                .index = GLOBAL_INDEX_TO_SUBPOOL_OFFSET(src_idx)
            };
            next_idx = expand_local_reference(tag);
            delete_reference(tag);

        } else if (next.index != 0) {
            next_idx = src_idx + next.index;
        }

        ((uint16_t*)dst_spool)[dst_sp_idx] = next_idx == REF_NOT_FOUND ? 0 : ONE_STEP;

        copy_data_fields(dst_spool,
                         src_spool,
                         dst_sp_idx,
                         src_sp_idx,
                         &field_offsets[1],
                         field_count -1);

        if (next_idx != REF_NOT_FOUND) {
            if (NULL_REF == pool_alloc(dst_pool))
                return OUT_OF_MEM;
            dst_idx++;
        }

        src_idx = next_idx;
    }

    return start_idx;
}


/* 
 * Places the tree copy by walking it in order
 * THIS FUNCTION CAN'T BE USED IN PARALLEL
 */

static global_reference
move_btree(pool_reference *dst_pool,
              pool_reference *src_pool,
              global_reference root)
{
    static size_t field_count;
    static pool_struct src;

    if (root == NULL_REF)
        return NULL_REF;

    src.raw_val = *src_pool;
    field_count = type_table[src.type_id].field_count;

    global_reference old_left = get_field_reference(root, 0);
    global_reference new_left = move_btree(dst_pool, src_pool, old_left);

    global_reference new_root = pool_alloc(dst_pool);
    for (size_t i = 2 ; i < field_count ; ++i)
        set_field(new_root, i, get_field(root, i));
    /*TODO: More efficient copying */

    set_field_reference(new_root, 0, new_left);

    global_reference old_right = get_field_reference(root, 1);
    global_reference new_right = move_btree(dst_pool, src_pool, old_right);

    set_field_reference(new_root, 1, new_right);

    return new_root;
}

/*
 * Move ntree and place it level by level in the new pool.
 */
static int 
move_ntree(pool_reference *dst_pool,
           pool_reference *src_pool,
           global_reference root,
           global_reference new_root,
           size_t field_count,
           size_t ref_field_count)
{
    for (size_t i = ref_field_count  ; i < field_count ; ++i) {
        set_field(new_root, i, get_field(root, i));
    }

    global_reference new_children[ref_field_count];
    global_reference children[ref_field_count];
    size_t child_count = 0;

    for (size_t i =  0 ; i < ref_field_count ; ++i) {
        global_reference child = get_field_reference(root, i);
        if (child != NULL_REF) {
            global_reference new_child = pool_alloc(dst_pool); 

            if (new_child == NULL_REF)
                return 1;

            set_field_reference(new_root, i, new_child);
            children[child_count] = child;
            new_children[child_count++] = new_child;
        }
    }

    for (size_t i = 0 ; i < child_count ; ++i)
        move_ntree(dst_pool,
                   src_pool,
                   children[i],
                   new_children[i],
                   field_count,
                   ref_field_count);
    return 0;
}

