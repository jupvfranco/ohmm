/**
 * @brief Functions and structures for a table aiming to expand local
 * references that have insufficient reach.
 *
 * The functions declared in this file are not meant to be used by "end users",
 * they are used internally by local-heaps, and are only of interest to people
 * wanting to modify the system.
 *
 * This translation does use a layer of indirection, and is thus expensive. If
 * pools in a program have to fall back on this method often, then something is
 * not working as intended. The current implementation uses a hash table.
 *
 * @file reference_table.h
 * @author Martin Hagelin
 * @date December, 2014
 *
 */

#ifndef __REFERECE_TABLE_H__
#define __REFERECE_TABLE_H__

#include "pool.h"
#include "pool_private.h"



/**
 * @brief Looks up a local reference in a table and returns an absolute index.
 *
 * Observe that having to use this function a lot indicates that something is
 * wrong, and that the pool might need to be compressed.
 *
 * @param key A reference_tag (a 64 bit integer unique identifier) representing
 *            the local reference to look up.
 *
 * @return An absolute index from the start of the pool, or REF_NOT_FOUND if
 *         there is no matching entry for key.
 */
size_t
expand_local_reference(reference_tag key);

/**
 * @brief Enters an absolute index in a table and returns a local reference.
 *
 * Note that all indexes stored away are currently very persistent, care must
 * be taken to be sure they are always removed when an object is removed.
 *
 * @param key A reference_tag under which the index should be stored.
 *
 * @param absolute_index The absolute index to store away.
 *
 * @return 0 on success.
 */
int
compress_absolute_index(reference_tag key, size_t absolute_index);

/**
 * @brief Removes a particular local reference from the table.
 *
 * @param key a reference_tag which should be removed.
 *
 * @return 0 on success.
 */
int
delete_reference(reference_tag key);

/**
 * @brief Remove all local reference entries for a certain pool.
 *
 * This function can be called when freeing a pool that contains a lot of
 * expanded local references. Please be aware that doing this is potentially
 * very expensive.  
 *
 * @param pool The pool for which all local reference expansions should be
 *             cleared.
 *
 * @return 0 on success.
 */
int
delete_all_for_pool(pool_reference pool);


/**
 * @brief All functions listed in this block are here ONLY FOR TESTING purposes.
 *
 * There is no guarantee that these symbols will even be exported in a
 * production version.
 */
#ifndef __RELEASE__

/**
 * @brief Internal function that produces a pseudo random 64 bit integer from a
 * global reference.
 *
 * NOT part of the public interface.
 * The function used is based on work done by Bob Jenkins and Thomas Wang, see
 * the report for references.
 *
 * @param global_ref A global reference to produce a hash for.
 * @return A 64 bit value, where the least significant bits should be suitable
 * to use as an index into a hash table.
 *
 */
uint64_t
hash_func(uint64_t key);

/**
 * @brief Internal helper function that grows the hash table used for lookups.
 *
 * @param new_size the new size of the table.
 * @return 0 on success.
 */
int
grow_hash_table(const size_t new_size);

/**
 * @brief Internal helper function that clears away all the deleted references
 * from the table.
 *
 * @return 0 on success.
 */
int
cleanup_hash_table(void);


#endif /* __RELEASE__ */

#endif
