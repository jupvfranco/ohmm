/**
 * @brief Simple benchmark for allocation speed.
 *
 * @file alloc_benchmark.c
 * @author Martin Hagelin
 * @date November, 2014
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "basic_types.h"
#include "../test/test_type_info.h"
//#include "type_info.h"
#include "pool.h"

#define DEFAULT_ITERATIONS 1000000LU
#define U_SEC_TO_SEC(t) (  ((double) (t/1000000)) + \
                           (((double) (t % 1000000)) / 1000000.0) )

static unsigned long long
profile_malloc_dynarray(const unsigned long iterations);

static unsigned long long
profile_malloc_single_chars(const unsigned long iterations);

static unsigned long long
profile_palloc_dynarray(const unsigned long iterations);

static unsigned long long
profile_palloc_single_chars(const unsigned long iterations);


int
main(int argc, char *argv[])
{
    add_basic_types();
    unsigned long iterations = DEFAULT_ITERATIONS;

    if (argc == 2) {
        if (1 != sscanf(argv[1], "%lu", &iterations)) {
            fprintf(stderr, "USAGE: %s [number of iterations]\n", argv[0]);
            return 1;
        }
    }

    unsigned long long malloc_time = profile_malloc_dynarray(iterations);
    unsigned long long palloc_time = profile_palloc_dynarray(iterations);


    printf( "\nTime for %lu interleaved allocations (dynamic arrays)\n"
            "\t%-22s %2.3lf s\n"
            "\t%-22s %2.3lf s\n"
            "\t%-22s %2.3lf times\n",
            iterations,
            "standard malloc:", U_SEC_TO_SEC(malloc_time),
            "pooled alloc:", U_SEC_TO_SEC(palloc_time),
            "speedup:", U_SEC_TO_SEC(malloc_time) / U_SEC_TO_SEC(palloc_time));

    malloc_time = profile_malloc_single_chars(iterations);
    palloc_time = profile_palloc_single_chars(iterations);

    printf( "\n\nTime for %lu discrete allocations (of chars)\n"
            "\t%-22s %2.3lf s\n"
            "\t%-22s %2.3lf s\n"
            "\t%-22s %2.3lf times\n",
            iterations,
            "standard malloc:", U_SEC_TO_SEC(malloc_time),
            "pooled alloc:", U_SEC_TO_SEC(palloc_time),
            "speedup:", U_SEC_TO_SEC(malloc_time) / U_SEC_TO_SEC(palloc_time));


	return 0;
}

static unsigned long long
profile_malloc_dynarray(const unsigned long iterations)
{
    struct timeval start;
    struct timeval stop;

    char *char_array_0 = NULL;
    char *char_array_1 = NULL;

    gettimeofday(&start, NULL);
    for (unsigned long i = 1 ; i <= iterations ; ++i) {
        char_array_0  = realloc(char_array_0, i);
        char_array_1  = realloc(char_array_1, i);
    }
    gettimeofday(&stop, NULL);


    free(char_array_0);
    free(char_array_1);

    return ((stop.tv_sec - start.tv_sec) * 1000000LLU) +
            stop.tv_usec - start.tv_usec; 
}

static unsigned long long
profile_palloc_dynarray(const unsigned long iterations)
{
    struct timeval start;
    struct timeval stop;

    pool_reference char_pool_0 = pool_create(CHAR_TYPE_ID);
    pool_reference char_pool_1 = pool_create(CHAR_TYPE_ID);

    gettimeofday(&start, NULL);
    for (unsigned long i = 0 ; i < iterations ; ++i) {
        pool_grow(&char_pool_0, 1);
        pool_grow(&char_pool_1, 1);
    }
    gettimeofday(&stop, NULL);

    pool_destroy(&char_pool_0);
    pool_destroy(&char_pool_1);

    return ((stop.tv_sec - start.tv_sec) * 1000000LLU) +
            stop.tv_usec - start.tv_usec; 
}

static unsigned long long
profile_malloc_single_chars(const unsigned long iterations)
{
    struct timeval start;
    struct timeval stop;

    char **char_ptrs = malloc(sizeof(char*) * iterations);

    gettimeofday(&start, NULL);
    for (unsigned long i = 0 ; i < iterations ; ++i) {
        char_ptrs[i] = malloc(sizeof(char));
    }
    gettimeofday(&stop, NULL);

    for (unsigned long i = 0 ; i < iterations ; ++i) {
        free(char_ptrs[i]);
    }

    free(char_ptrs);

    return ((stop.tv_sec - start.tv_sec) * 1000000LLU) +
            stop.tv_usec - start.tv_usec; 
}

static unsigned long long
profile_palloc_single_chars(const unsigned long iterations)
{
    struct timeval start;
    struct timeval stop;

    pool_reference char_ref_pool = pool_create(CHAR_REF_TYPE_ID);
    pool_reference char_pool = pool_create(CHAR_TYPE_ID);

    pool_grow(&char_ref_pool, iterations);
    
    global_reference *char_refs = pool_to_array(char_ref_pool);

    gettimeofday(&start, NULL);
    for (unsigned long i = 0 ; i < iterations ; ++i) {
        char_refs[i] = pool_alloc(&char_pool);
    }
    gettimeofday(&stop, NULL);

    pool_destroy(&char_ref_pool);
    pool_destroy(&char_pool);

    return ((stop.tv_sec - start.tv_sec) * 1000000LLU) +
            stop.tv_usec - start.tv_usec; 
}
