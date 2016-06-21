#ifndef __TEST_TYPE_INFO_H__
#define __TEST_TYPE_INFO_H__

#include "basic_types.h"
#include "field_info.h"
#include "type_info.h"
#include "CUnit/Basic.h"

/**
 * @brief Registers the basic set of types described in basic_types.h.
 *
 * The strange coupling between this test and the basic types is clearly
 * undesireable, and should be corrected at some point. In a testing phase it
 * is convenient however, as tests and benchmarks can use the same set of
 * types.
 *
 * @return 0 on success.
 */
int
add_basic_types(void);

size_t
fill_in_offsets(Field_offsets offsets, Type_info type, size_t *base_offset);

void
get_size_and_field_count(Type_info type, size_t *size, size_t *count);

void
t_fill_in_offsets(void);

void
t_get_size_and_field_count(void);

void
t_init_type_table(void);


#endif
