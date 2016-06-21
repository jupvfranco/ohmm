#include <cstdlib>
#include <cstdint>
#include <sys/time.h>
#include <map>

#include "benchmark_stl_tree.h"

#define BIGGER_THAN_L3 40000000LLU
#define SS_TO_USEC(START, STOP) ((STOP.tv_sec - START.tv_sec) * 1000000LLU) + \
                                  STOP.tv_usec - START.tv_usec; 


char other_data[BIGGER_THAN_L3];

struct time_measurements {
        unsigned long long	insert;
        unsigned long long	lookup;
};

static void
flush_cash(void);

unsigned long long
profile_stl_tree(  void *tm_struct,
                   size_t size,
                   size_t lookup_len  )
{
    srandom(0xdeadbeef);
    struct timeval start;
    struct timeval stop;
    auto *tm =(struct time_measurements*) tm_struct;

    unsigned long long lookup_keys[lookup_len];

    std::map<unsigned long long, unsigned long long> tree;

    gettimeofday(&start, NULL);
    for (size_t i = 0 ; i < size ; ++i) {
        tree[random()] = 0;
    }

    for (size_t i = 0 ; i < lookup_len ; ++i) {
        unsigned long long key = random();
        tree[key] = i;
        lookup_keys[i] = key;
    }

    gettimeofday(&stop, NULL);
    tm->insert = SS_TO_USEC(start, stop);
    flush_cash();

    unsigned long long sum = 0;
    gettimeofday(&start, NULL);
    for (size_t i = 0 ; i < lookup_len ; ++i) {
        sum += tree[lookup_keys[i]];
    }
    gettimeofday(&stop, NULL);

    tm->lookup = SS_TO_USEC(start, stop);

    return sum;
}


void
flush_cash(void)
{
    for (size_t i = 0 ; i < BIGGER_THAN_L3 ; ++i)
        other_data[i] = (other_data[i] + i) & 0xff;
}
