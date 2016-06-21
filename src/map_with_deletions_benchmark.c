/**
 * @brief A set of microbenchmarks for mapping over lists where elements have
 * been deleted.
 *
 * Compares the speed of a map over simple linked lists with linked lists
 * allocated using local-heaps. 
 *
 * Various percentages of the original lists are deleted before the benchmark
 * is run.
 *
 * @file map_with_deletions_benchmark.c
 * @author Martin Hagelin
 * @date January, 2015
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "benchmark_vector_map.h"
#include "linked_list.h"
#include "pool.h"
#include "pool_map.h"
#include "pool_iterator.h"
#include "gc.h"
#include "basic_types.h"
#include "../test/test_type_info.h"

#define DEFAULT_LENGTH 200000
#define DEFAULT_DELETE_PROBABILITY 0.5

#define U_SEC_TO_SEC(t) (  ((double) ((t)/1000000)) + \
                           (((double) ((t) % 1000000)) / 1000000.0) )
#define SS_TO_USEC(START, STOP) ((STOP.tv_sec - START.tv_sec) * 1000000LLU) + \
                                  STOP.tv_usec - START.tv_usec; 

#define BIGGER_THAN_L3 40000000LLU

struct time_measurements {
	unsigned long long	create;
	unsigned long long	del;
	unsigned long long	map;
	unsigned long long	gc;
	unsigned long long	map_after_gc;
};

char other_data[BIGGER_THAN_L3];

void
flush_cash(void);

void
square(void *x, void *y);

void
profile_simple_list_map(struct time_measurements *tm, const unsigned long length, double deletion);

void
profile_pooled_list_map(struct time_measurements *tm, unsigned long length, double deletion);

unsigned long long
deletion_overhead_time(size_t length);

static int
print_usage(char *program_name)
{
    fprintf(stderr, "USAGE: %s [list-length] [deletion-probability]\n",
            program_name);
    return 1;
}


int
main(int argc, char *argv[])
{
    add_basic_types();
    gc_init();
    unsigned long length = DEFAULT_LENGTH;
    double delete_probability = DEFAULT_DELETE_PROBABILITY;

    if (argc > 3)
        return print_usage(argv[0]);

    if (argc > 1) {
        if (1 != sscanf(argv[1], "%lu", &length))
            return print_usage(argv[0]);
    }

    if (argc > 2) {
        if (1 != sscanf(argv[2], "%lf", &delete_probability))
            return print_usage(argv[0]);
    }

    printf( "List length: %lu\n"
            "Deletion probability: %lf\n",
            length, delete_probability);

    struct time_measurements lt;
    struct time_measurements pt;
    struct time_measurements vt;


    uint64_t del_time = deletion_overhead_time(length);
    profile_pooled_list_map(&pt, length, delete_probability);
    profile_simple_list_map(&lt, length, delete_probability);
    profile_vector_map((void*)&vt, length, delete_probability, del_time);

    printf( "\n\nTime needed to map over a list taking deletions into account\n"
            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\n"

            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\n"

            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\n"
            "\n"

            "\t%-32s %2.3lf times\n"
            "\t%-32s %2.3lf times\n"
            "\t%-32s %2.3lf times\n"
            "\n"
            "\t%-32s %2.3lf times\n"
            "\t%-32s %2.3lf times\n"
            "\t%-32s %2.3lf times\n"

            "\n"
            "\t%-32s %2.3lf times\n"
            "\t%-32s %2.3lf times\n"
            "\n"
            "\t%-32s %2.3lf times\n"
            "\t%-32s %2.3lf times\n",

            "simple list creation time:", U_SEC_TO_SEC(lt.create),
            "simple list deletion time:", U_SEC_TO_SEC(lt.del),
            "simple list map:", U_SEC_TO_SEC(lt.map),

            "std::vector creation time:", U_SEC_TO_SEC(vt.create),
            "std::vector deletion time:", U_SEC_TO_SEC(vt.del),
            "std::vector map:", U_SEC_TO_SEC(vt.map),

            "pooled list creation time:", U_SEC_TO_SEC(pt.create),
            "pooled list deletion time:", U_SEC_TO_SEC(pt.del),
            "pooled list gc:", U_SEC_TO_SEC(pt.gc),
            "pooled list map before gc:", U_SEC_TO_SEC(pt.map),
            "pooled list map after gc:", U_SEC_TO_SEC(pt.map_after_gc),

            "speedup vs vector before gc:",
            U_SEC_TO_SEC(vt.map) / U_SEC_TO_SEC((pt.map)),

            "speedup vs vector after gc:",
            U_SEC_TO_SEC(vt.map) / U_SEC_TO_SEC((pt.map_after_gc)),

            "speedup vs vector including gc:",
            U_SEC_TO_SEC(vt.map) / U_SEC_TO_SEC((pt.map_after_gc + pt.gc)),



            "speedup vs list before gc:",
            U_SEC_TO_SEC(lt.map) / U_SEC_TO_SEC(pt.map),

            "speedup vs list after gc:",
            U_SEC_TO_SEC(lt.map) / U_SEC_TO_SEC(pt.map_after_gc),

            "speedup vs list including gc:",
            U_SEC_TO_SEC(lt.map) / U_SEC_TO_SEC((pt.map_after_gc + pt.gc)),

            "total speedup vs vector, excluding gc:",
            U_SEC_TO_SEC(vt.create + vt.del + vt.map) /
            U_SEC_TO_SEC(pt.create + pt.del + pt.map),

            "total speedup vs vector, including gc:",
            U_SEC_TO_SEC(vt.create + vt.del + vt.map) /
            U_SEC_TO_SEC(pt.create + pt.del + pt.gc + pt.map_after_gc),

            "total speedup vs list, excluding gc:",
            U_SEC_TO_SEC(lt.create + lt.del + lt.map) /
            U_SEC_TO_SEC(pt.create + pt.del + pt.map),

            "total speedup vs list, including gc:",
            U_SEC_TO_SEC(lt.create + lt.del + lt.map) /
            U_SEC_TO_SEC(pt.create + pt.del + pt.map + pt.gc)

    );


    return 0;
}




void
flush_cash(void)
{
    for (size_t i = 0 ; i < BIGGER_THAN_L3 ; ++i)
        other_data[i] = (other_data[i] + i) & 0xff;
}

void
profile_pooled_list_map(struct time_measurements *tm, const unsigned long length, double deletion)
{
    srandom(0xdeadbeef);
    struct timeval start;
    struct timeval stop;

    int64_t del_threshold = RAND_MAX*deletion;

    pool_reference result_pool = pool_create(LONG_TYPE_ID);

    gettimeofday(&start, NULL);
    pool_reference list_pool = pool_create(LIST_TYPE_ID);
    global_reference head = pool_alloc(&list_pool);
    pool_iterator itr = iterator_new(&list_pool, &head);

    for (size_t i = 1 ; i < length; ++i) {
        iterator_list_insert(itr, pool_alloc(&list_pool));
        itr = iterator_next(list_pool, itr);
        iterator_set_field(itr, 1, &i);
    }
    gettimeofday(&stop, NULL);

    tm->create = SS_TO_USEC(start, stop);

    flush_cash();

    gettimeofday(&start, NULL);
    itr = iterator_new(&list_pool, &head);
    while (itr != ITERATOR_END) {
        if (random() < del_threshold) {
            if (0 != iterator_list_remove(itr))
                break;
        } else {
            itr = iterator_next(list_pool, itr);
        }
    }
    gettimeofday(&stop, NULL);

    tm->del= SS_TO_USEC(start, stop) - deletion_overhead_time(length);

    flush_cash();

    gettimeofday(&start, NULL);
    field_list_map(head, &result_pool, 1, square);
    gettimeofday(&stop, NULL);

    tm->map = SS_TO_USEC(start, stop);

    flush_cash();

    gettimeofday(&start, NULL);
    push_root(&head);
    collect_pool(&list_pool);
    gettimeofday(&stop, NULL);

    tm->gc = SS_TO_USEC(start, stop);

    gettimeofday(&start, NULL);
    field_map(list_pool, &result_pool, 1, square);
    gettimeofday(&stop, NULL);

    tm->map_after_gc = SS_TO_USEC(start, stop);

    pool_destroy(&list_pool);
    pool_destroy(&result_pool);

    return; 
}

void
profile_simple_list_map(struct time_measurements *tm, unsigned long length, double deletion)
{
    srandom(0xdeadbeef);
    struct timeval start;
    struct timeval stop;
    size_t new_len = length;

    int64_t del_threshold = RAND_MAX*deletion;

    Node head = new_node(NULL);
    head->a = 0;

    gettimeofday(&start, NULL);
    Node cursor = head;
    for (size_t i = 1 ; i < length ; ++i) {
        Node n = new_node(NULL);
        n->a = i;
        cursor->next = n;
        cursor = cursor->next;
    }
    gettimeofday(&stop, NULL);

    tm->create = (( stop.tv_sec - start.tv_sec) * 1000000LLU) +
		            stop.tv_usec - start.tv_usec; 

    flush_cash();

    gettimeofday(&start, NULL);
    for (Node n = head ; n != NULL ; ) {
        if (random() < del_threshold && n->next != NULL) {
            Node t = n->next;
            n->next = n->next->next;
            free(t);
            new_len--;
        } else {
            n = n->next;
        }
    }
    gettimeofday(&stop, NULL);

    tm->del= (( stop.tv_sec - start.tv_sec) * 1000000LLU) +
		            stop.tv_usec - start.tv_usec - deletion_overhead_time(length); 

    flush_cash();

    gettimeofday(&start, NULL);
    uint64_t *result = list_field_map(head, new_len, square);
    gettimeofday(&stop, NULL);

    tm->map = ((stop.tv_sec - start.tv_sec) * 1000000LLU) +
                stop.tv_usec - start.tv_usec;

    destroy_list(head);
    free(result);

    return;
}

void
square(void *x, void *y)
{
    *((uint64_t*)y) = (*((uint64_t*)x)) * (*((uint64_t*)x));
}

unsigned long long
deletion_overhead_time(size_t length)
{
    return 0;
    static unsigned long long overhead;
    if (overhead != 0)
        return overhead;

    srandom(0xdeadbeef);
    struct timeval start;
    struct timeval stop;

    gettimeofday(&start, NULL);
    for (size_t i = 0 ; i < length ; ++i)
        (void) random();
    gettimeofday(&stop, NULL);

    overhead = ((stop.tv_sec - start.tv_sec) * 1000000LLU) +
                 stop.tv_usec - start.tv_usec; 

    return overhead;
}
