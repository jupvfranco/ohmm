/**
 * @brief Declares runtime type information used by the local-heap project.
 *
 * Provides structures that can be used to give a high level representation of
 * types, as well as a function that interprets this representation and builds
 * a type table.
 *
 * See the test cases for extra information of how the structures can be used.
 *
 * @file type_info.h
 * @author Martin Hagelin
 * @date October, 2014
 *
 */

#ifndef __TYPE_INFO_H__
#define __TYPE_INFO_H__

#include <stdint.h>
#include <stdlib.h>


/**
 * @brief What type of a type is the type? 
 *
 * Primitive types are regarded by the runtime as indivisible units, they may
 * be simple characters or doubles, or a megabyte blob of binary data.
 *
 * Composite types will be split automatically by the runtime, and most likely
 * allocated in different memory sub-pools.
 *
 */
typedef enum type_class {
    PRIMITIVE_TYPE,
    COMPOSITE_TYPE,

    /* 
     * TODO: Make into a type instead, as local references can only be of the
     * same type as the pool  they are in
     */
    LOCAL_REF_TYPE, 

    GLOBAL_REF_TYPE
} TYPE_CLASS;


/**
 * @brief An array of high level type information.
 */
typedef struct type_info const *Type_info;

/**
 * @brief Contains information about the layout of a data type.
 *
 * The type_id should be unique integer assigned by the compiler.
 * the ids MUST be consecutive as they are designed to be used
 * as indexes to a type table.
 */
struct type_info {
   uint16_t             type_id;        /* 16 bits for now */
   TYPE_CLASS           type_class;

   union {
       uint64_t         referee_type_id;
       size_t           field_count;
       size_t           primitive_size;
   };

   const Type_info      fields[0];
};

/**
 * @brief Initializes meta data structures needed by the runtime.
 *
 * Pre-calculates relative offsets to different fields and other 
 * information needed to minimize overhead for accesses and allocations.
 *
 * In future versions it would be recommended to do this at compile time
 * instead.
 *
 * @param type_count The number of elements in the type_infos array.
 * @param type_infos An array of all data types that can be dynamically
 *                   allocated by the program.
 * @return 0 on success.
 */
int
init_type_table(int type_count, Type_info type_infos[]);

#endif
