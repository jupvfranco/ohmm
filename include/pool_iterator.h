/**
 * @brief Iterators for local-heaps pools.
 *
 * These iterators are of a generic nature, and as such they can be on the
 * expensive side to use. If it is at all possible it is better to follow the
 * pattern in pool_map when accessing and manipulating the data in a pool.
 *
 * Users should also be aware that the interface towards the different types of
 * iterators declared here may be slightly different as the codebase is still
 * in a state of flux. Refer to the test cases and benchmarks for example
 * usage.
 *
 *
 * @file pool_iterator.h
 * @author Martin Hagelin
 * @date December 2014
 *
 */

#ifndef __POOL_ITERATOR_H__
#define __POOL_ITERATOR_H__

#include "pool.h"

#define NULL_ITERATOR 0u
#define ITERATOR_END NULL_ITERATOR

/**
 * @brief An iterator reference, should not be modified except by local-heaps.
 */
typedef uint64_t pool_iterator;


/**
 * @brief Creates a new iterator for a pool of composite objects.
 *
 * Assumes there is a head or root element at index 0 in the pool. This
 * function is most suited for pools that only contain a single structure.
 * Observe that this function is considered for obsolescence as iterator_new
 * can perform the same function.
 *
 * @param pool A valid pool_reference.
 * @return A pool_iterator pointing at the head of the pool.
 */
pool_iterator
iterator_from_pool(pool_reference pool);

/**
 * @brief Creates a new iterator for a pool of composite objects.
 *
 * The use of this function is the recommended way of creating iterators.
 *
 * @param pool A pointer to a pool reference, it may be a NULL_REF if the pool
 * contains list objects. 
 *
 * @param root A pointer to the root or head object of a linked structure. It
 * may be NULL_POOL if the pool does not contain linked objects.
 *
 * @return A pool_iterator pointing to the root object, or the first object in
 * the pool if successful, or NULL_ITERATOR on failure.
 */
pool_iterator
iterator_new(pool_reference *pool, global_reference *root);

/**
 * @brief Destroys an iterator, freeing all memory used by it.
 *
 * It is not strictly necessary to call this function if the iterator is of the
 * simple type used for linked lists and arrays, but it does no harm.
 *
 * @param iterator A pointer to the iterator to destroy.
 * @return nothing.
 */
void
iterator_destroy(pool_iterator *iterator);

/**
 * @brief Creates a new iterator for a pool P, starting with the object
 * referred to by reference ref. 
 *
 * This function is considered for obsolescence as iterator_new can perform the
 * same task.
 *
 * @param ref a valid reference to an object O in a pool P.
 * @return a pool_iterator pointing to the object O, or NULL_ITERATOR on error.
 */
pool_iterator
iterator_from_reference(global_reference ref);

/**
 * @brief   Moves the iterator cursor to the next element in the
 *          collection.
 *
 * @param pool_ref A valid reference to a pool containing non linked objects.
 * @param iterator A valid iterator associated with the pool 'pool_ref'.
 * @return  A pool_iterator belonging to the next element in the pool, if no
 * such element exists, ITERATOR_END is returned.
 */
pool_iterator
iterator_next(pool_reference pool_ref, pool_iterator iterator);

/**
 * @brief Moves the iterator to the previous element in the pool.
 *
 * @param pool_ref A valid reference to a pool containing non linked ojbects.
 * @param iterator A valid iterator associated with the pool 'pool_ref'.
 * @return A pool_iterator belonging to the previous element in the pool, if no
 * such element exists, ITERATOR_END is returned.
 */
pool_iterator
iterator_prev(pool_iterator iterator);

/** 
 * @brief Returns a pointer to the contents of a field belonging to a certain
 * iterator.
 *
 * At the moment this functions does the exact same thing as the get_field
 * function defined in pool.h, but this will change as iterators are made more
 * general.
 *
 * @param iterator A valid iterator.
 * @param field The index of the field to access.
 * @return A pointer to field number 'field' in the element pointed to by
 * 'iterator', if iterator is at the end of it's pool, then NULL is returned.
 */
void*
iterator_get_field(pool_iterator iterator, const size_t field);

/**
 * @brief Sets a particular field of the object at the current cursor of an
 * iterator.
 *
 * At the moment this functions do the exact same thing as the set_field
 * function defined in pool.h, but this will change as iterators are made more
 * general.
 *
 * @param iterator A valid, iterator that is NOT at the end of a pool.
 * @param field The index of the field to update.
 * @param data A pointer to the new data to place in field #'field'.
 *
 * @return 0 on success.
 *
 */
int
iterator_set_field( pool_iterator iterator,
                    const size_t field,
                    const void *data);


/**
 * @brief Inserts the object referred to by reference AFTER the object referred
 * to by iterator. 
 *
 * The given reference and iterator must both refer to the same memory pool.
 * As the name implies this function will only work as intended for iterators
 * over pools containing lists.
 *
 * @param iterator An iterator for a pool P, containing single linked objects
 * of type T.
 *
 * @param reference a reference to an object of type T residing in pool P.
 *
 * @return 0 on success.
 */
int
iterator_list_insert(pool_iterator iterator, global_reference reference);

/**
 * @brief Removes the element AFTER the element pointed to by iterator.
 *
 * The simple structure of a list iterator prevents deletion of objects before
 * the current element.
 *
 * @param iterator a list iterator pointing to an element X.
 * @return 0 if the element after X was successfully removed.
 */
int
iterator_list_remove(pool_iterator iterator);


/*
 * The helper functions below are not exported except in debug mode, and are
 * included here only for testing.
 */
#ifndef __RELEASE__

/**
 * @brief Internal helper function used by local-heaps.
 */
global_reference
pool_get_ref(pool_reference pool, size_t index);

/**
 * @brief Internal helper function used by local-heaps.
 */
pool_iterator
iterator_simple_next(pool_reference pool_ref, pool_iterator iterator);

/**
 * @brief Internal helper function used by local-heaps.
 */
pool_iterator
iterator_list_next(pool_iterator iterator);

/**
 * @brief Internal helper function used by local-heaps.
 */
pool_iterator
iterator_tree_next(pool_iterator iterator);
#endif

#endif
