/**
 * @brief Declares functions for pooled allocations.
 *
 * All programs using local-heaps will want to include this file. It declares
 * all functions needed to create, grow, shrink and destroy pools. There are
 * also functions to modify and retrieve data from pools.
 * Users should be aware that there is a significant latency involved when
 * going through these functions to retrieve many individual objects. If at all
 * possible custom functions following the pattern in pool_map should be
 * generated.
 *
 * @file pool.h
 * @author Martin Hagelin
 * @date October, 2014
 *
 */

#ifndef __POOL_H__
#define __POOL_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>
#include <string.h>


#define NULL_POOL 0u
#define NULL_REF 0u


/**
 * @brief Simple macro to check if a reference is NULL.
 */
#define REF_IS_NULL(ref) (ref.raw_val == NULL_REFl)

/**
 * @brief Opaque reference to a specific memory pool. It should only be altered
 *        by local-heaps.
 */
typedef uint64_t pool_reference;

/**
 * @brief Opaque reference to an object.
 */
typedef uint64_t global_reference;


/**
 * @brief Creates a new memory pool for objects of type T.
 *
 * It is imperative that the type information for the type requested has been
 * registered before this function is called.
 *
 * @param type_id The unique identifier of a type T.
 *
 * @return A Pool on success, NULL_POOL on failure. 
 */
pool_reference
pool_create(uint16_t type_id);

/**
 * @brief Destroys (frees) an entire pool.
 *
 * There is no need to shrink or free the individual objects stored in the pool
 * before calling this function.
 * As a side effect the given reference is set to NULL_REF.
 *
 * @param pool The pool to free.
 * @return 0 on success.
 */
int
pool_destroy(pool_reference *pool);

/**
 * @brief Allocates memory for an additional object in a pool.
 *
 * @param pool	A pointer to a pool of type T in which to allocate space for
 *              another object.
 *
 * @return A global reference to the allocated memory on success,
 *         NULL_REF on failure.
 */
global_reference
pool_alloc(pool_reference *pool);

/**
 * @brief Grows a pool to accommodate num_elements more elements.
 *
 * This function is meant to be used for dynamically growing arrays and similar
 * things, not for the allocation of list nodes etc, as no global_reference is
 * returned.
 *
 * @param pool	A pointer to the pool that should be grown.
 * @param num_elements
 * 
 * @return 0 on success.
 */
int
pool_grow(pool_reference *pool, const size_t num_elements);

/**
 * @brief Shrinks a pool as less elements are needed.
 *
 * @param pool A pointer to the pool that should be shrunk.
 * @param num_elements The number of elements the pool should be shrunk with.
 *
 *
 * @return 0 on success.
 */
int
pool_shrink(pool_reference *pool, const size_t num_elements);

/**
 * @brief Returns a pointer to field inside an object.
 *
 * This function is relatively expensive.  When looping over many objects and
 * fields other means of access should be used.
 * It is possible, but dangerous to alter the returned void pointer, doing so
 * WILL update the pool. Just be very careful not to write more bytes than the
 * size of the given field.
 *
 * @param reference	A reference to an object.
 * @param field_nr	The field number to access in the object.
 *
 * @return A pointer to field number field_nr in the object referred to by
 *         reference, or NULL on failure.
 */
void*
get_field(const global_reference reference, const size_t field_nr);

/**
 * @brief Sets the field of an object.
 *
 * This is an expensive way of updating fields, when doing many sets other
 * means of access should be used if possible. The supplied data is copied, and
 * guaranteed not to be altered.
 *
 * @param reference	A reference to the object to update.
 * @param field_nr	The number of the field to access.
 * @param data		A pointer to the new data to store in the object.
 *
 * @return 0 on success.
 *
 */
int
set_field(const global_reference reference, 
          const size_t field_nr, 
          const void* data);

/**
 * @brief Sets a reference field in a pool element.
 *
 * As pools may only contain local references the argument, which is a global
 * reference will be converted to a local reference.
 * Note that this function will fail if the reference being set is not pointing
 * to an object in the same pool.
 *
 * @param this_ref A reference to the object that should be updated.
 * @param field_nr The number of the field to update, this should be a local
 * reference field.  
 * @param that_ref A reference or iterator to the object that the field should
 * point to. 
 * @return Zero on success.
 */
int
set_field_reference(const global_reference this_ref,
                    const size_t field_nr,
                    const global_reference that_ref); 

/**
 * @brief Expands the local reference at field_nr in object referred to by
 * this_ref into a global reference.
 *
 * @param this_ref a global reference to a linked object O.
 * @param field_nr a field number in which O stores a local reference L.
 *
 * @return A global reference referring to the same object as L.
 */
global_reference
get_field_reference(const global_reference this_ref,
                    const size_t field_nr);

/**
 * @brief Returns a pointer to the start of the memory area used by a pool.
 *
 * This function is meant to be used for dynamic arrays of non composite types.
 * If a pool is used this way IT MUST NOT be used for anything else!
 *
 * @param pool The pool to be used as a dynamic array.
 * @return A pointer to the start of the memory area to use, NULL on failure.
 */
void*
pool_to_array(const pool_reference pool);
#endif
