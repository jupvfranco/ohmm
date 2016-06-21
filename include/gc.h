/**
 * @brief Declares functions used for collecting and compressing heaps.
 *
 * This is a very basic garbage collection system, and for the time being it
 * has no support for arbitrary graphs. It is also prohibited for a pool to
 * contain global references to other pools. If the system runs out of memory
 * during a collection it may result in catastrophic failure, and terminating
 * the program is recommended if a collection fails..
 *
 * @file gc.h
 * @author Martin Hagelin
 * @date Decmber, 2014
 *
 */

#ifndef __GC_H__
#define __GC_H__

#include "basic_types.h"
#include "field_info.h"
#include "pool.h"
#include "pool_iterator.h"
#include "pool_private.h"
#include "reference_table.h"

/**
 * @brief Initializes data structures used by the garbage collector.
 *
 * Call this function before attempting to perform a collection, it will
 * allocate a small pool to store a stack when called. The overhead of this is
 * small, but if the GC is not used, then simply avoid calling this function.
 *
 * @return 0 on success.
 */
int
gc_init(void);

/**
 * @brief Collects and compresses a pool of memory.
 *
 * The references pointed to by pool and roots WILL be changed as elements are
 * moved. If this function fails, then it's imperative that the program
 * execution is halted, a junk state might have been entered.
 *
 * @param pool A pointer to the pool to be collected.
 * @return 0 on success.
 */
int
collect_pool(pool_reference *pool);

/**
 * @brief Will push a reference that's part of the root set in preparation for
 * collection.
 *
 * Note that only one pool should be collected at a time, meaning that
 * references to different pools should never be pushed before successfully
 * calling collect_pool().
 *
 * @param root a pointer to the reference that should be pushed.
 * 
 * @return 0 on success.
 */
int
push_root(global_reference *root);


#endif
