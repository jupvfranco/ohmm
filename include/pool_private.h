/**
 * @brief Contains macro and structure declarations that aren't part of the
 *        public interface.
 *
 *  The structures and macros declared here are mostly of interest to people
 *  wanting to modify local-heaps. They are NOT part of the public interface,
 *  and documentation is on a best effort basis.
 *
 * @file pool_private.h
 * @author Martin Hagelin
 * @date December 2013
 * 
 */

#ifndef __POOL_PRIVATE_H__
#define __POOL_PRIVATE_H__

#include <stdlib.h>
#include <stdint.h>

/**
 * @brief The page size on the current architecture, it is uses synonymously
 * with the length of a sub-pool.
 */
#define PAGE_SIZE ((size_t) 1 << 12)

/**
 * @brief A very platform specific address where local-heaps will start to
 * allocate pools.
 *
 * Change this if the operating system you're using sets up different memory
 * mappings than linux does.
 */
#define POOL_START ((void*) (1LLU << 32))
/**
 * @brief A very platform specific address where local-heaps will stop to
 * allocate pools.
 *
 * Change this if the operating system you're using sets up very different
 * memory mappings than linux does.
 */
#define POOL_STOP  ((void*) (0x7001 << 32 ))

/**
 * @brief Returns the base address for a pool.
 * @param IDX A unique pool id.
 * @return The address to the start of the pool, in the form of an integer.
 */
#define POOL_IDX_TO_ADDR(IDX) (((uintptr_t) IDX) << 32 )

/**
 * @brief Returns the base address of a pool reference.
 * @param ref A reference to a pool or an object in a pool.
 * @return An address to the start of the pool, in the form of an integer.
 */
#define GET_POOL_ADDR(ref) (((uintptr_t)(ref).pool_id) << 32)

/**
 * @brief Helpful macro to calculate how many extra subpools are needed for a
 * specific number of objects.
 *
 * This macro is basically on obfuscated way of dividing a number by 4096 and
 * rounding up. A better compiler than gcc would make this unnecessary, but 4.8
 * does not implement divisions by larger powers of two as a bit shift.
 *
 * @param SIZE The number of objects to be allocated.
 * @return The number of subpools needed to allocate. 
 */
#define SUB_POOLS_NEEDED(SIZE) ((!!((SIZE) & ((1 << 12) -1))) + ((SIZE) >> 12))

/**
 * @brief Gets the size of a subpool in bytes.
 *
 * @param ref A reference to a pool or object of type T.
 * @return The size of a subpool in a T-pool.
 */
#define GET_SUB_POOL_SIZE(ref) (type_table[(ref).type_id].type_size * PAGE_SIZE)

/**
 * @brief The absolute index of an object, counting from the top of the pool.
 * @param ref A reference to object number N in a pool.
 * @return N.
 */
#define GET_GLOBAL_INDEX_OF_REF(ref) ((ref).sub_pool_id*PAGE_SIZE + (ref).index)

/**
 * @brief The number of objects in a pool.
 * @param pool Reference to a pool.
 * @return The number of objects allocated in pool (including garbage).
 */
#define GET_SIZE_OF_POOL(pool) ((pool).sub_pool_id*PAGE_SIZE + (pool).index + \
                               ((pool).full && (pool).index == 0? PAGE_SIZE: 0))

/**
 * @brief Given a reference (or pool), and a defined type table:
 * Get the offset from the start of a subpool where the field_nr fields start.
 *
 * @param ref A global reference to an object in a pool.
 * @param field_nr The specific field number to access.
 * @return The offset into a subpool where field number field_nr resides.
 */
#define GET_FIELD_OFFSET(ref, field_nr) \
    (type_table[(ref).type_id].field_offsets[field_nr].offset * PAGE_SIZE)
/**
 * @brief Gets the size of a given field in bytes.
 *
 * @param ref A reference to a pool or an object in a pool.
 * @param field_nr The number of the field to get the size of.
 * @return The size, in bytes, of field number field_nr.
 */
#define GET_FIELD_SIZE(ref, field_nr) \
    (type_table[(ref).type_id].field_offsets[field_nr].field_size)

/**
 * @brief Gets the subpool given an absolute index.
 *
 * @param idx The absolute index of an object O, counted from the top of the pool
 *            it resides in.
 * @return The subpool O resides in.
 */
#define GLOBAL_INDEX_TO_SUBPOOL_ID(idx) ((idx) >> 12)

/**
 * @brief Gets the index into a subpool.
 *
 * @param idx The absolute index of an object O, counted from the top of the
 *            pool it resides in.
 * @return The index of object O, counted from the top of the subpool it
 *         resides in.
 */
#define GLOBAL_INDEX_TO_SUBPOOL_OFFSET(idx) ((idx) & ((1 << 12) - 1))

/**
 * @brief A "magic" absolute index that represents a non existing object.
 *
 * Sometimes references are represented with an index internally, for example
 * in iterators in order to save some operations. It is assumed that the most
 * significant bit will never be needed to represent an index, so when it's set
 * it has a special meaning internally.
 */
#define REF_NOT_FOUND ( ~((size_t) 0u) )

/**
 * @brief A "magic" index used to indicate the start of pool. It is used by
 *       iterators internally.
 */
#define REF_BEGIN 0x7000000000000000

/**
 * @brief A "magic" index used to indicate the end of a pool, It is used
 *        internally by iterators.
 */
#define REF_END REF_NOT_FOUND


/**
 * @brief A convenience macro that can be used to test whether a reference is
 * in valid. 
 *
 * All magic reference valid begin have the most significant bit set. In order
 * to check if a reference is valid it is therefore possible to do a logic and
 * with this value.
 */
#define REF_INVALID_BIT REF_BEGIN




/*
 * Functions declared as PRIVATE will be static in the release version.
 * Not before however, as this would make them very hard to test.
 */
#ifdef __RELEASE__

#define PRIVATE static
#define INLINED static inline
#else

/**
 * @brief A macro that is defined as static when the code is not being tested.
 */
#define PRIVATE
/**
 * @brief A macro that is defined as inline static when the code is not being
 *        tested.
 */
#define INLINED
#endif

/**
 * @brief Opaque local reference to an object, not part of the external
 * interface since local references should never leave the pool.
 */
typedef uint16_t local_reference;

/**
 * @brief Internal representation of a local reference.
 */
typedef struct local_reference {
    union {
        uint16_t        raw_val;
        struct {
            int         index        : 13;
            unsigned    is_long_ref  : 1;
            unsigned    gc_state     : 2;
        };
    } __attribute__((packed));
} local_reference_struct;

/**
 * @brief The internal representation of a global reference.
 */
typedef struct global_reference {
    union {
        uint64_t                raw_val;
        struct {
            uint16_t            type_id;
            uint16_t            sub_pool_id;
            uint16_t            pool_id;
            union {
                uint16_t        raw_index;
                struct { 
                    unsigned    index       : 12;
                    unsigned    reserved    : 1;
                    unsigned    is_extended : 1;
                    unsigned    gc_state    : 2;
                };
            };
        } __attribute__((packed));
    };
} reference_struct;

/**
 * @brief Internal representation of a pool reference.
 *
 * There is no guarantee this will retain the same structure as the
 * global_reference struct.  But doing so has the advantage that many of the
 * above macros can be used for both types of references.
 */
typedef struct pool_reference {
    union {
        uint64_t                raw_val;
        struct {
            uint16_t            type_id;
            uint16_t            sub_pool_id;
            uint16_t            pool_id;
            union {
                uint16_t        raw_index;
                struct {
                    unsigned    index       : 12;
                    unsigned    full        : 1;
                    unsigned    is_extended : 1;
                    unsigned    gc_state    : 2;    /* Use these to represent */
                };                                  /* packed state? */
            };
        } __attribute__((packed));
    };
} pool_struct;

/**
 * @brief The various types of iterators used by local-heaps.
 *
 */
typedef enum iterator_type {
    ITERATOR_SIMPLE             = 0,
    ITERATOR_LIST               = 1,
    ITERATOR_LIST_COMPACT       = 2,
    ITERATOR_COMPLEX            = 3,
} ITERATOR_TYPE;

/**
 * @brief The representation of a SIMPLE iterator.
 *
 *
 * No guarantee this will retain the same structure as 
 * the global_reference struct. 
 * But it's convenient for now.
 */
typedef struct pool_iterator {
    union {
        uint64_t                raw_val;
        struct {
            uint16_t            type_id;
            uint16_t            sub_pool_id;
            uint16_t            pool_id;
            union {
                uint16_t        raw_index;
                struct {
                    unsigned    index         : 12;
                    unsigned    full          : 1;
                    unsigned    is_extended   : 1;
                    ITERATOR_TYPE iterator_type : 2;
                };
            };
        } __attribute__((packed));
    };
} iterator_struct;

/**
 * @brief The representation of a COMPLEX iterator.
 *
 * This struct is really placed in a pool of 64 bit integers that is used as a
 * dynamic array. The stack part is allowed to grow as large as it needs to,
 * and us used to keep track of the path taken when traversing tree structures.
 */
typedef struct complex_iterator {
    uint64_t    iter_pool;      /* Iterator pool */

    uint64_t   *root;           
    uint64_t   *pool;           /* Iterated pool */
    void       *pool_start;     /* Cached address */
    size_t      elem_size;
    size_t      num_children;

    size_t      prev;
    size_t      cursor;
    size_t      next;

    size_t      n;              /* Current size of stack */
    size_t      stack[0];
} complex_iterator_struct;


/**
 * @brief The structure of the numbers used to look up local references  in the
 *        global hash table.
 */
typedef struct reference_tag {
    union {
        uint64_t                raw_val;
        struct {
            uint16_t            local_ref;
            uint16_t            sub_pool_id;
            uint16_t            pool_id;
            uint16_t            index;
        } __attribute__((packed));
    } __attribute__((packed));
} __attribute__((packed)) reference_tag;


#endif
