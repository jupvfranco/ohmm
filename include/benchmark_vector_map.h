#ifndef __BENCHMARK_VECTOR_MAP_H__
#define __BENCHMARK_VECTOR_MAP_H__

#ifdef __cplusplus
extern "C" void
#else
void
#endif
profile_vector_map(void *tm_struct,
                   const unsigned long length,
                   double deletion, 
                   const unsigned long long del_overhead);

#endif

