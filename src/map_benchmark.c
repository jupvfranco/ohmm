/** 
 * @brief A set of benchmarks for mapping over lists.
 *
 * Compares the speed of a map with simple operations over simple linked lists
 * with linked lists allocated using local-heaps. 
 *
 * Various levels of fragmentation can be used, this is simulated by inserting
 * an unrelated allocation during the construction of the list.
 *
 * @file map_benchmark.c
 * @author Martin Hagelin
 * @date January, 2015
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "linked_list.h"
#include "pool.h"
#include "pool_map.h"
#include "pool_iterator.h"
#include "basic_types.h"
#include "../test/test_type_info.h"

#define DEFAULT_LENGTH 200000
#define DEFAULT_FRAGMENTATION 0.5
#define U_SEC_TO_SEC(t) (  ((double) (t/1000000)) + \
                           (((double) (t % 1000000)) / 1000000.0) )

#define BIGGER_THAN_L3 40000000LLU

char other_data[BIGGER_THAN_L3];

uint64_t*
array_field_map(Node src,
                size_t length,
                void (*f)(void *, void*));

void
square(void *x, void*y);

static unsigned long long
profile_simple_list_map(const unsigned long size, double fragmentation);

static unsigned long long
profile_pooled_list_map(const unsigned long size, double fragmentation);

static unsigned long long
profile_array_map(const unsigned long size);

void
flush_cash(void);

static int
print_usage(char *program_name)
{
    fprintf(stderr, "USAGE: %s [list-length] [fragmentation-probability]\n",
            program_name);
    return 1;
}

int
main(int argc, char *argv[])
{
    add_basic_types();
    unsigned long size = DEFAULT_LENGTH;
    double fragmentation = DEFAULT_FRAGMENTATION;

    if (argc > 3)
        return print_usage(argv[0]);

    if (argc > 1) {
        if (1 != sscanf(argv[1], "%lu", &size))
            return print_usage(argv[0]);
    }

    if (argc > 2) {
        if (1 != sscanf(argv[2], "%lf", &fragmentation))
            return print_usage(argv[0]);
    }

    printf( "List length: %lu\n"
            "Framentation probability: %lf\n",
            size, fragmentation);

    uint64_t pooled_time = profile_pooled_list_map(size, fragmentation);
    uint64_t list_time = profile_simple_list_map(size, fragmentation);
    uint64_t array_time = profile_array_map(size);

    printf( "\n\nTime needed to map over a list %lu elements long\n"
            "\t%-22s %2.3lf s\n"
            "\t%-22s %2.3lf s\n"
            "\t%-22s %2.3lf s\n"
            "\t%-22s %2.3lf times\n"
            "\t%-22s %2.3lf times\n",
            size,
            "simple array map:", U_SEC_TO_SEC(array_time),
            "simple list map:", U_SEC_TO_SEC(list_time),
            "pooled list map:", U_SEC_TO_SEC(pooled_time),
            "speedup vs list:",
            U_SEC_TO_SEC(list_time) / U_SEC_TO_SEC(pooled_time),
            "speedup vs array:",
            U_SEC_TO_SEC(array_time) / U_SEC_TO_SEC(pooled_time));

    return 0;
}

static unsigned long long
profile_pooled_list_map(const unsigned long size, double fragmentation)
{
    srandom(0xdeadbeef);
    struct timeval start;
    struct timeval stop;

    int64_t frag_threshold = RAND_MAX*fragmentation;
    Node head_unrelated_alloc = new_unrelated(NULL, 2.0);

    pool_reference list_pool = pool_create(LIST_TYPE_ID);
    pool_reference result_pool = pool_create(LONG_TYPE_ID);
    global_reference head = pool_alloc(&list_pool);
    pool_iterator itr = iterator_new(&list_pool, &head);

    for (size_t i = 1 ; i < size ; ++i) {
        if (random() < frag_threshold)
            new_unrelated(head_unrelated_alloc, 2.0);

        const uint64_t foo = 42;
        iterator_list_insert(itr, pool_alloc(&list_pool));
        itr = iterator_next(list_pool, itr);

        iterator_set_field(itr, 1, &i);
        iterator_set_field(itr, 2, &foo);
    }

    flush_cash();

    gettimeofday(&start, NULL);
    field_map(list_pool, &result_pool, 1, square);
    gettimeofday(&stop, NULL);

    pool_destroy(&list_pool);
    pool_destroy(&result_pool);

    return ((stop.tv_sec - start.tv_sec) * 1000000LLU) +
            stop.tv_usec - start.tv_usec; 
}

static unsigned long long
profile_simple_list_map(const unsigned long size, double fragmentation)
{
    srandom(0xdeadbeef);
    struct timeval start;
    struct timeval stop;

    Node head_unrelated_alloc = new_unrelated(NULL, 10.0);

    Node head = new_node(NULL);
    head->a = 0;
    int64_t frag_threshold = RAND_MAX*fragmentation;
    for (size_t i = 1 ; i < size ; ++i) {
        if (random() < frag_threshold)
            new_unrelated(head_unrelated_alloc, 10.0);

        head = new_node(head);
        head->a = i;
        head->b = 42;
    }

    flush_cash();

    gettimeofday(&start, NULL);
    uint64_t *result = list_field_map(head, size, square);
    gettimeofday(&stop, NULL);

    destroy_list(head_unrelated_alloc);
    destroy_list(head);
    free(result);

    return ((stop.tv_sec - start.tv_sec) * 1000000LLU) +
            stop.tv_usec - start.tv_usec; 
}

static unsigned long long
profile_array_map(const unsigned long size)
{
    struct timeval start;
    struct timeval stop;

    Node node_array = malloc(sizeof(struct node) * size);
    for (size_t i = 0 ; i < size ; ++i) {
        node_array[i].a = i;
        node_array[i].b = 42;
    }

    flush_cash();

    gettimeofday(&start, NULL);
    uint64_t *result = array_field_map(node_array, size, square);
    gettimeofday(&stop, NULL);

    free(node_array);
    free(result);
    return ((stop.tv_sec - start.tv_sec) * 1000000LLU) +
            stop.tv_usec - start.tv_usec; 
}

uint64_t*
array_field_map(Node src,
                size_t length,
                void (*f)(void *, void*))
{
    uint64_t *result = malloc(sizeof(uint64_t) * length);
    if (result == NULL)
        return NULL;

    for (size_t i = 0 ; i < length ; ++i) {
        f(&src[i].a, &result[i]);
    }
    return result;
}

void
square(void *x, void *y)
{
    *((uint64_t*)y) = (*((uint64_t*)x)) * (*((uint64_t*)x));
}

void
flush_cash(void)
{
    for (size_t i = 0 ; i < BIGGER_THAN_L3 ; ++i)
        other_data[i] = (other_data[i] + i) & 0xff;
}
