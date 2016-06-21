#ifndef __BENCHMARK_STL_TREE
#define __BENCHMARK_STL_TREE


#ifdef __cplusplus
#include <cstdlib>
extern "C" unsigned long long
#else
#include <stdlib.h>
unsigned long long
#endif
profile_stl_tree(  void *tm_struct,
                   size_t size,
                   size_t lookup_len  );

#endif

