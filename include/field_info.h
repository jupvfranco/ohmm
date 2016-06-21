/**
 * @brief Implementation specific representation of type tables.
 *
 * The information in this file is not part of the public interface and may
 * change at any time. It may however be of interest to anyone wanting to
 * extend or modify the local-heaps system. Some work remains, the
 * representation could for instance be more compact.
 *
 * @file field_info.h
 * @author Martin Hagelin
 * @date November, 2014
 */

#ifndef __FIELD_INFO_H__
#define __FIELD_INFO_H__

#include <stdlib.h>
#include <stdint.h>

#include "type_info.h"

/**
 * @brief Contains precaluclated offsets for a data type.
 *
 * This information is calculated from the type_info generated
 * by the compiler.  For the time being this is done at runtime, but in future
 * versions it should be done at compile time. Using 64 bit integers to
 * represent sizes is doubtless overkill, but in a testing phase it rules out
 * overflow errors.
 */
typedef struct field_offset { 
    uint16_t    type_id;
    size_t      field_size;     /* Size of field in bytes */
    size_t      offset;         /* Offset into object, in bytes */
} *Field_offsets;

/**
 * @brief A (rather inefficient) representation of a type table.
 *
 * This information is what allows localheaps to structure pools and to split
 * objects into parts
 */
typedef struct type_offsets {
    uint16_t            type_class;
    uint16_t            referee_type_id;
    size_t              type_size;
    size_t              field_count;
    struct field_offset *field_offsets;
} *Type_table;

#ifndef __RELEASE__
/* 
 * Two functions declared in type_info.c that are NOT part of the public
 * interface of local-heaps. In release mode the symbols are not guaranteed to
 * be exported 
 */

/**
 * @brief Function used internally by local heaps to help fill in type table.
 *
 * There is no reason to call this function for end users, it should only be
 * used by local-heaps during the construction of the type table. Specifically
 * it is called by the init_type_table() function declared in type_info.h
 *
 * @param offsets The field_offset array that should be updated.
 * @param type The type for which the information should be written.
 * @param base_offset How many bytes into a composite object that the current
 *                    type of object resides. 
 */
size_t
fill_in_offsets(Field_offsets offsets, Type_info type, size_t *base_offset);

/** 
 * @brief Function used internally by local heaps to calculate the size of a
 * composite type. 
 *
 * There is no reason for end users to call this function. It is used by
 * init_type_table() to calculate the size of a composite type.
 *
 * @param type High level structure of the type that should be analyzed.
 * @param size A pointer to an integer where the size will be written.
 * @param count A pointer to an integer where the number of fields will be
 * written, including all "flattened" fields.
 */
void
get_size_and_field_count(Type_info type, size_t *size, size_t *count);
#endif 

#endif
