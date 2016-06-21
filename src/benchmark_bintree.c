/**
 * @brief A set of microbenchmarks intended to measure the overhead and latency of 
 * accessing individual elements in a pool. Specifically this is done using a
 * binary search tree.
 *
 * @file benchmark_bintree.c
 * @author Martin Hagelin
 * @date January, 2015
 */

#define DEFAULT_SIZE 200000
#define DEFAULT_LOOKUP_SIZE 20000

#define U_SEC_TO_SEC(t) (  ((double) ((t)/1000000)) + \
                           (((double) ((t) % 1000000)) / 1000000.0) )
#define SS_TO_USEC(START, STOP) ((STOP.tv_sec - START.tv_sec) * 1000000LLU) + \
                                  STOP.tv_usec - START.tv_usec; 

#define BIGGER_THAN_L3 40000000LLU

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "linked_list.h"
#include "pool.h"
#include "basic_types.h"
#include "../test/test_type_info.h"

#include "benchmark_stl_tree.h"

struct time_measurements {
	unsigned long long	insert;
	unsigned long long	lookup;
};

char other_data[BIGGER_THAN_L3];

void
flush_cash(void);

static void
insert(pool_reference *pool, global_reference root, uint64_t key, uint64_t value);

uint64_t*
lookup(global_reference root, uint64_t key);

uint64_t
profile_bintree(struct time_measurements *tm, size_t size, size_t lookup_size);

static int
print_usage(char *program_name)
{
    fprintf(stderr, "USAGE: %s [tree_size] [lookup_size]\n",
            program_name);
    return 1;
}

int
main(int argc, char *argv[])
{
    add_basic_types();
    size_t size = DEFAULT_SIZE;
    size_t lookup_size = DEFAULT_LOOKUP_SIZE;

    if (argc > 3)
        return print_usage(argv[0]);

    if (argc > 1) {
        if (1 != sscanf(argv[1], "%lu", &size))
            return print_usage(argv[0]);
    }

    if (argc > 2) {
        if (1 != sscanf(argv[2], "%lu", &lookup_size))
            return print_usage(argv[0]);
    }

    struct time_measurements pt;
    struct time_measurements st;

    profile_bintree(&pt, size, lookup_size);
    profile_stl_tree(&st, size, lookup_size);

    printf( "\n\nTime to insert and lookup elements in binary tree\n"
            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\t%-32s %2.3lf s\n"
            "\n"
//            "\t%-32s %2.3lf times\n"
//            "\t%-32s %2.3lf times\n"
           ,"Binary tree in pool, insertion: ", U_SEC_TO_SEC(pt.insert)
           ,"Binary tree in pool, lookup: ", U_SEC_TO_SEC(pt.lookup)
           ,"std::map insertion: ", U_SEC_TO_SEC(st.insert)
           ,"std::map lookup: ", U_SEC_TO_SEC(st.lookup)
    );

}




static void
insert(pool_reference *pool, global_reference root, uint64_t key, uint64_t value)
{
   uint64_t *k = get_field(root, 2);

   if (*k < key) {
       global_reference left = get_field_reference(root, 0);
       if (left != NULL_REF) {
           insert(pool, left, key, value);
       } else {
           left = pool_alloc(pool);
           set_field(left, 2, &key);
           set_field(left, 3, &value);
           set_field_reference(root, 0, left);
       }
   } else if (*k > key) {
       global_reference right = get_field_reference(root, 1);
       if (right != NULL_REF) {
           insert(pool, right, key, value);
       } else {
           right = pool_alloc(pool);
           set_field(right, 2, &key);
           set_field(right, 3, &value);
           set_field_reference(root, 1, right);
       }
   } else {
       set_field(root, 3, &value);
   }
}

uint64_t*
lookup(global_reference root, uint64_t key)
{
    if (NULL_REF == root)
        return NULL;

    uint64_t *k = get_field(root, 2);
    if (*k  < key) {
        return lookup(get_field_reference(root, 0), key);
    } else if(*k > key) {
        return lookup(get_field_reference(root, 1), key);
    } else {
        return get_field(root, 3);
    }
}

uint64_t
profile_bintree(struct time_measurements *tm, size_t size, size_t lookup_size)
{
	uint64_t lookup_keys[lookup_size];

    srandom(0xdeadbeef);
    struct timeval start;
    struct timeval stop;

    int64_t root_val = RAND_MAX / 2;
    pool_reference tree_pool = pool_create(BTREE_TYPE_ID);
    global_reference root = pool_alloc(&tree_pool);
    set_field(root, 2, &root_val);


    gettimeofday(&start, NULL);
    for (size_t i = 0 ; i < size ; ++i) {
        insert(&tree_pool, root, random(), 0);
    }

    for (size_t i = 0 ; i < lookup_size ; ++i) {
        uint64_t key = random();
        insert(&tree_pool, root, key, i);
        lookup_keys[i] = key;
    }
    gettimeofday(&stop, NULL);

    tm->insert = SS_TO_USEC(start, stop);

    flush_cash();

    uint64_t sum = 0;
    gettimeofday(&start, NULL);
    for (size_t i = 0 ; i < lookup_size ; ++i) {
         sum += *lookup(root, lookup_keys[i]);
    }
    gettimeofday(&stop, NULL);

    tm->lookup = SS_TO_USEC(start, stop);

    pool_destroy(&tree_pool);

    return sum;
}

void
flush_cash(void)
{
    for (size_t i = 0 ; i < BIGGER_THAN_L3 ; ++i)
        other_data[i] = (other_data[i] + i) & 0xff;
}
