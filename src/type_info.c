/**
 * @brief Definitions for functions used to set up run-time representations of
 *        types.
 * 
 * In future versions of this memory management system this information will be
 * generated at compile time, but for now all programs using it will be reqired
 * to run the init_type_table function before using the allocation functions.
 * This is needed to populate the read only tables used by the mm-system to
 * calculate the correct offsets into pools and sub-pools.
 *
 * @file type_info.c
 * @author Martin Hagelin
 * @date November, 2014
 */
#include <sys/mman.h>
#include <errno.h>

#include "type_info.h"
#include "field_info.h"
#include "pool.h"
#include "pool_private.h"


typedef uint16_t local_reference;

/* Defined in pool.c */
extern Type_table type_table;

PRIVATE size_t
fill_in_offsets(Field_offsets offsets, Type_info type, size_t *base_offset)
{
    switch (type->type_class) {
        case PRIMITIVE_TYPE:
            offsets->type_id    = type->type_id;
            offsets->field_size = type->primitive_size;
            break;
        case GLOBAL_REF_TYPE:
            offsets->type_id    = type->type_id;
            offsets->field_size = sizeof(global_reference);
            break;
        case LOCAL_REF_TYPE:
            offsets->type_id    = type->type_id;
            offsets->field_size = sizeof(local_reference);
            break;
        case COMPOSITE_TYPE: 
            {
                size_t field_count = 0u;
                for (unsigned i = 0 ; i < type->field_count ; ++i ) {
                field_count += fill_in_offsets( &offsets[field_count], 
                                                type->fields[i], 
                                                base_offset);
                }
                return field_count;
            }
    }

    offsets->offset = *base_offset;
    *base_offset += offsets->field_size;
    return 1u; /* Primitive types have a field count of 1 */
}

PRIVATE void
get_size_and_field_count(Type_info type, size_t *size, size_t *count)
{
    *count = 1;

    if (type->type_class == PRIMITIVE_TYPE ) {
        *size = type->primitive_size;
        return;
    }

    if (type->type_class == GLOBAL_REF_TYPE ) {
        *size  = sizeof(global_reference);
        return;
    }

    if (type->type_class == LOCAL_REF_TYPE ) {
        *size = sizeof(local_reference);
        return;
    }

    *count = 0;
    *size = 0;
    for (unsigned i = 0 ; i < type->field_count ; ++i) {
        size_t fs, fc;
        get_size_and_field_count(type->fields[i], &fs, &fc);
        *size  += fs;
        *count += fc;
    }
}

int
init_type_table(int type_count, Type_info type_infos[]) 
{
    assert(type_count < (1<<16));

    size_t table_size = type_count*sizeof(struct type_offsets);
    Type_table tt = mmap(0, 
                         table_size, 
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS,
                         0, 0);

    if (tt == MAP_FAILED)
        return errno;

    size_t total_count = 0;
    for (int i = 0 ; i < type_count ; ++i) {
        size_t size, count;
        get_size_and_field_count(type_infos[i], &size, &count);
        tt[i].type_size = size;
        tt[i].field_count = count;
        tt[i].type_class = type_infos[i]->type_class;

        if (LOCAL_REF_TYPE == tt[i].type_class ||
            GLOBAL_REF_TYPE == tt[i].type_class ) {
            tt[i].referee_type_id = (uint16_t) type_infos[i]->referee_type_id;
        }
            
        total_count += count;
    }

    size_t field_offset_table_size = total_count*sizeof(struct field_offset);

    Field_offsets offsets = mmap(0, 
                                 field_offset_table_size,
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_ANONYMOUS,
                                 0, 0);

    if (offsets == MAP_FAILED)
        return errno;

    size_t j = 0;
    for (int i = 0 ; i < type_count ; ++i) {
        size_t offset = 0;
        size_t tmp = fill_in_offsets(&offsets[j], type_infos[i], &offset);
        tt[i].field_offsets = &offsets[j];
        j += tmp;
    }

    if (0 != mprotect(tt, table_size, PROT_READ))
        return errno;

    if (0 != mprotect(offsets, field_offset_table_size, PROT_READ))
        return errno;

    type_table = tt;

    return 0;
}

