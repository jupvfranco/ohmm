#include <cstdlib>
#include <cstdint>
#include <sys/time.h>
#include <iterator>
#include <vector>

#include "benchmark_vector_map.h"

#define DELETE_CHEAT

#define BIGGER_THAN_L3 40000000LLU
#define SS_TO_USEC(START, STOP) ((STOP.tv_sec - START.tv_sec) * 1000000LLU) + \
                                  STOP.tv_usec - START.tv_usec; 

std::vector<uint64_t>
map_vector_field(std::vector<struct node> &A, void (*f) (void*, void*));

char other_data[BIGGER_THAN_L3];

struct time_measurements {
        unsigned long long	create;
        unsigned long long	del;
        unsigned long long	map;
        unsigned long long	gc;
        unsigned long long	map_after_gc;
};

struct node {
    uint64_t a;
    uint64_t b;
};

void
square(void *x, void*y);

static void
flush_cash(void);

void
profile_vector_map(void *tm_struct,
                   const unsigned long length,
                   double deletion, 
                   const unsigned long long del_overhead)
{
    srandom(0xdeadbeef);
    struct timeval start;
    struct timeval stop;
    auto *tm =(struct time_measurements*) tm_struct;

    int64_t del_threshold = RAND_MAX*deletion;

    gettimeofday(&start, NULL);
    std::vector<struct node> list;
    for (size_t i = 0 ; i < length ; ++i) {
        struct node n = {.a = i , .b = 42 };
        list.push_back(n);
    }
    gettimeofday(&stop, NULL);

    tm->create = SS_TO_USEC(start, stop);

    flush_cash();
    
    gettimeofday(&start, NULL);

#ifdef DELETE_CHEAT
    std::vector<struct node> new_list;
    for (auto i : list) {
        if (random() > del_threshold)
            new_list.push_back(i);
    }
    list = new_list;
#else
    /*
     * Nobody in their right mind would do a delete like this on a vector, but
     * it is the right comparision as this micro benchmark simulates a large
     * number of deletes spread out through program execution.
     *
     */
    for (auto i = list.begin() ; i != list.end() ; ) {
        if (random() < del_threshold)
            i = list.erase(i);
        else 
            ++i;
    }
#endif

    gettimeofday(&stop, NULL);

    tm->del = SS_TO_USEC(start, stop) - del_overhead;

    flush_cash();

    gettimeofday(&start, NULL);
    auto result = map_vector_field(list, square);
    gettimeofday(&stop, NULL);

    tm->map = SS_TO_USEC(start, stop);
}

std::vector<uint64_t>
map_vector_field(std::vector<struct node> &A, void (*f) (void*, void*)){
    std::vector<uint64_t> B;

    for (auto a : A) {
        uint64_t b;
        square(&a.a, &b);
        B.push_back(b);
    }

    return B;
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

