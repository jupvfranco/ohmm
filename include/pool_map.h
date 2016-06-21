/**
 * @brief Declarations for a basic map framework for pools.
 *
 * The functions here describe the preferred usage of the local-heaps system.
 * Iterating over an entire pool and doing some work on individual fields is
 * the use case for which the system is optimized.
 *
 * All cases are clearly not covered here, field_map and other functions are
 * meant to be used as inspiration and templates for other specially generated
 * functions.
 *
 * @file pool_map.h
 * @author Martin Hagelin
 * @date January 2015
 *
 */

#ifndef __POOL_MAP_H__
#define __POOL_MAP_H__

#include "pool.h"

typedef void (*map_function_type) (void*, void*);

/**
 * @brief Applies a function to a specific field of every element in a pool.
 *
 * At the moment this function assumes a compact pool.
 * (No deletions can have been made since the last compactation).
 *
 * @param A A pool containing elements of type a, where a has a field number
 * field_no of type b.
 *
 * @param B A pointer to an empty pool of type b.
 *
 * @param f A function a -> b, where a pointer to an a-element is given as the
 * first argument, and a pointer to a b-element is given as the second
 * agrument.
 *
 * @returns 0 on success.
 */
int
field_map(const pool_reference A,
          pool_reference *B,
          size_t field_no,
          map_function_type f);

/**
 * @brief Applies a function to a specific field of every element in a list.
 *
 * As opposed to field_map this function makes no assumption that the pool has
 * been compacted or that no deletions have been made, it will work regardless.
 * But if the pool really IS compact, then it is MUCH better to use a function
 * like field_map instead, as it's much more efficient.
 *
 * @param A A reference to the head of a list where each node is an object that
 * has a field numbe rfield_no of type a.
 *
 * @param B A pointer to an empty pool of type b.
 *
 * @param f A function a -> b, where a pointer to an a-element is given as the
 * first argument, and a pointer to a b-element is given as the second
 * agrument.
 *
 * @returns 0 on success.
 */
int
field_list_map(const global_reference A,
	           pool_reference *B,
	           size_t field_no,
	           map_function_type f);

#endif
