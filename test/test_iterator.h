#ifndef __TEST_ITERATOR_H__
#define __TEST_ITERATOR_H__

#include "CUnit/Basic.h"
#include "pool.h"
#include "pool_iterator.h"
#include "pool_private.h"
#include "field_info.h"

void
t_pool_get_ref(void);

void
t_iterator_simple_next_and_prev(void);

void
t_iterator_list_next(void);

void
t_iterator_list_insert(void);

void
t_iterator_list_remove(void);

void
t_iterator_btree(void);

void
t_iterator_ntree(void);

#endif
