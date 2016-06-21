#ifndef __TEST_POOL_H__
#define __TEST_POOL_H__

#include "CUnit/Basic.h"
#include "pool.h"
#include "pool_private.h"
#include "field_info.h"

void
t_pool_create(void);

void
t_pool_alloc(void);

void
t_set_field(void);

void
t_get_field(void);

void
t_set_and_get_field_reference(void);

void
t_pool_grow(void);

void
t_pool_shrink(void);

void
t_pool_destroy(void);

#endif
