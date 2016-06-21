#ifndef __TEST_GC_H__
#define __TEST_GC_H__

#include "CUnit/Basic.h"

#include "gc.h"
#include "pool_iterator.h"
#include "pool_private.h"
#include "field_info.h"

void
t_collect_list_pool(void);

void
t_collect_btree_pool(void);

void
t_collect_ntree_pool(void);

#endif
