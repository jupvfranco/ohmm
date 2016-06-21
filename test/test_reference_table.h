#ifndef __TEST_REFERENCE_TABLE_H__
#define __TEST_REFERENCE_TABLE_H__

#include "CUnit/Basic.h"
#include "reference_table.h"

void
t_expand_and_compress_local_reference(void);

void
t_grow_hash_table(void);

void
t_cleanup_hash_table(void);

void
t_delete_reference(void);

void
t_delete_all_for_pool(void);


#endif
