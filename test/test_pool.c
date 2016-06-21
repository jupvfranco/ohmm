#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>

#include "test_pool.h"

/* For pool_get_ref TODO: see if that func should be moved */
#include "pool_iterator.h"

/* 
 * These test-functions assume that a type table has been initialized with the
 * following types:
 *
 * Type 0: 1 byte char
 * Type 1: 8 byte double or 64 bit int 
 * Type 2: 8 byte reference
 * Type 3: Composite type, char + char + char + double
 * Type 4: Composite type, Double + Type 3 + Type 3
 *
 */
#include "basic_types.h"

#define UNUSED(x) ((void)x)

static sigjmp_buf context;
static void 
segv_handler(int sig_num)
{
    UNUSED(sig_num);
    siglongjmp(context, 1);
}

static pool_reference pool_0 = NULL_POOL;
static pool_reference pool_1 = NULL_POOL;
static pool_reference pool_2 = NULL_POOL;


void
t_pool_create(void)
{
    pool_0 = pool_create(0u);
    CU_ASSERT_NOT_EQUAL_FATAL(pool_0, NULL_POOL);

    void (*old_handler)(int) = signal(SIGSEGV, segv_handler);

    /* 
     * Testing that the entire allocated page is writable
     * Observe that this is a test of the internal representation. 
     * NOT A USAGE EXAMPLE (unless you like code exploding in your face)
     */
    void *p0_addr =  (void*) (pool_0 & ((( 1llu << 16 ) - 1) << 32));

    if (sigsetjmp(context, 1) == 0) {
        memset(p0_addr, 0xff, PAGE_SIZE);
        CU_PASS("Write to allocated memory ok");
    } else {
        CU_FAIL("Memory access to allocated memory segfaulted");
    }

    /* 
     * These accesses are suppsed to segfault, if you're looking here because
     * of angry alarms from Valgrind, be less alarmed 
     */
    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p0_addr) - 1) = 0xff;
        CU_FAIL("Able to access memory before start of pool");
    } else {
        CU_PASS("Memory access before allocated memory prevented");
    }

    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p0_addr) + PAGE_SIZE) = 0xff;
        CU_FAIL("Able to access memory after end of pool");
    } else {
        CU_PASS("Memory access after allocated memory prevented");
    }

    (void) signal(SIGSEGV, old_handler);
}

void
t_pool_alloc(void)
{
    /* Assuming this is run after t_pool_create */
    CU_ASSERT_NOT_EQUAL_FATAL(pool_0, NULL_POOL);

    void *p0_addr =  (void*) (pool_0 & ((( 1llu << 16 ) - 1) << 32));

    int number_of_null_refs = 0;
    for (unsigned i = 0 ; i < PAGE_SIZE ; ++i) {
        global_reference g0 = pool_alloc(&pool_0);
        number_of_null_refs += g0 == NULL_REF ? 1 : 0;
    }
    CU_ASSERT_EQUAL(number_of_null_refs, 0);

    void (*old_handler)(int) = signal(SIGSEGV, segv_handler);

    if (sigsetjmp(context, 1) == 0) {
        memset(p0_addr, 0xff, PAGE_SIZE);
        CU_PASS("Write to allocated memory ok");
    } else {
        CU_FAIL("Memory access to allocated memory segfaulted");
    }

    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p0_addr) + PAGE_SIZE) = 0xff;
        CU_FAIL("Additional page allocated unnecessarily"); 
    } else {
        CU_PASS("No additional memory allocated unnecessarily");
    }

    number_of_null_refs = 0;
    for (unsigned i = 0 ; i < PAGE_SIZE ; ++i) {
        global_reference g0 = pool_alloc(&pool_0);
        number_of_null_refs += g0 == NULL_REF ? 1 : 0;
    }
    CU_ASSERT_EQUAL(number_of_null_refs, 0);

    if (sigsetjmp(context, 1) == 0) {
        memset(p0_addr, 0xff, PAGE_SIZE*2);
        CU_PASS("Write to allocated memory ok");
    } else {
        CU_FAIL("Memory access to allocated memory segfaulted");
    }

    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p0_addr) + PAGE_SIZE*2) = 0xff;
        CU_FAIL("Additional page allocated unnecessarily"); 
    } else {
        CU_PASS("No additional memory allocated unnecessarily");
    }


    (void) signal(SIGSEGV, old_handler);
}

void
t_set_field(void)
{
    pool_1 = pool_create(LONG_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(pool_1, NULL_POOL);
    

    void *p1_addr =  (void*) (pool_1 & ((( 1llu << 16 ) - 1) << 32));

    int number_of_failed_writes = 0;
    for (unsigned i = 0 ; i < PAGE_SIZE ; ++i) {
        uint64_t x = i;
        global_reference g_ref = pool_alloc(&pool_1);
        set_field(g_ref, 0, &x);
        uint64_t *y = ((uint64_t*) p1_addr) + i;
        number_of_failed_writes += x == *y ? 0 : 1;
    }
    CU_ASSERT_EQUAL(number_of_failed_writes, 0);

    number_of_failed_writes = 0;
    for (unsigned i = 0 ; i < PAGE_SIZE ; ++i) {
        uint64_t x = i + 0xdeadbeef00000000;
        global_reference g_ref = pool_alloc(&pool_1);
        set_field(g_ref, 0, &x);
        uint64_t *y = ((uint64_t*) p1_addr) + i + PAGE_SIZE;
        number_of_failed_writes += x == *y ? 0 : 1;
    }
    CU_ASSERT_EQUAL(number_of_failed_writes, 0);
}

void
t_get_field(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(pool_1, NULL_POOL);

    int number_of_failed_reads = 0;
    for (unsigned i = 0 ; i < PAGE_SIZE*10 ; ++i) {
        const uint64_t x = 0xbabeface00000000 + i;
        global_reference g_ref = pool_alloc(&pool_1);
        set_field(g_ref, 0, &x);
        uint64_t *y = get_field(g_ref, 0);
        number_of_failed_reads+= x == *y ? 0 : 1;
    }
    CU_ASSERT_EQUAL(number_of_failed_reads, 0);
}


void
t_pool_grow(void)
{
    pool_2  = pool_create(0u);
    CU_ASSERT_NOT_EQUAL(pool_2, NULL_POOL);

    void *p2_addr =  (void*) (pool_2 & ((( 1llu << 16 ) - 1) << 32));
    
    pool_grow(&pool_2, PAGE_SIZE);
    CU_ASSERT_NOT_EQUAL(pool_2, NULL_POOL);

    void (*old_handler)(int) = signal(SIGSEGV, segv_handler);

    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p2_addr) + PAGE_SIZE) = 0xff;
        CU_FAIL("Additional page allocated unnecessarily"); 
    } else {
        CU_PASS("No additional memory allocated unnecessarily");
    }

    pool_grow(&pool_2, 1);
    CU_ASSERT_NOT_EQUAL(pool_2, NULL_POOL);

    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p2_addr) + PAGE_SIZE) = 0xff;
        CU_PASS("Write to allocated memory ok");
    } else {
        CU_FAIL("Memory access to allocated memory segfaulted");
    }

    pool_grow(&pool_2, PAGE_SIZE*10 - 1);
    CU_ASSERT_NOT_EQUAL(pool_2, NULL_POOL);

    if (sigsetjmp(context, 1) == 0) {
        memset(p2_addr, 0xff, PAGE_SIZE*11);
        CU_PASS("Write to allocated memory ok");
    } else {
        CU_FAIL("Memory access to allocated memory segfaulted");
    }

    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p2_addr) + PAGE_SIZE*11) = 0xff;
        CU_FAIL("Additional page allocated unnecessarily"); 
    } else {
        CU_PASS("No additional memory allocated unnecessarily");
    }

    (void) signal(SIGSEGV, old_handler);
}

void
t_pool_shrink(void)
{
    /* Pool 2 should contain 11 pages of chars from previous test */
    CU_ASSERT_NOT_EQUAL_FATAL(pool_2, NULL_POOL);

    void *p2_addr =  (void*) (pool_2 & ((( 1llu << 16 ) - 1) << 32));

    CU_ASSERT_EQUAL(pool_shrink(&pool_2, PAGE_SIZE - 1), 0);
    /* Pool 2 should still contain 11 pages of chars */
    
    void (*old_handler)(int) = signal(SIGSEGV, segv_handler);

    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p2_addr) + PAGE_SIZE*10) = 0xff;
        CU_PASS("Write to allocated memory ok");
    } else {
        CU_FAIL("Memory access to allocated memory segfaulted");
    }

    CU_ASSERT_EQUAL(pool_shrink(&pool_2, 1), 0);
    /* Now one page should be released */

    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p2_addr) + PAGE_SIZE*10) = 0xff;
        CU_FAIL("Memory wasn't unmapped after shrink");
    } else {
        CU_PASS("Memory was unmapped after shrink");
    }


    CU_ASSERT_EQUAL(pool_shrink(&pool_2, PAGE_SIZE*5 -1), 0);

    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p2_addr) + PAGE_SIZE*5 - 1) = 0xff;
        CU_PASS("Write to allocated memory ok");
    } else {
        CU_FAIL("Memory access to allocated memory segfaulted");
    }

    /* 6 pages freed, allocating them again */
    CU_ASSERT_EQUAL(pool_grow(&pool_2, 1+PAGE_SIZE*6), 0);

    if (sigsetjmp(context, 1) == 0) {
        memset(p2_addr, 0xff, PAGE_SIZE*11);
        CU_PASS("Write to allocated memory ok");
    } else {
        CU_FAIL("Memory access to allocated memory segfaulted");
    }
    
    if (sigsetjmp(context, 1) == 0) {
        *(((char*)p2_addr) + PAGE_SIZE*11) = 0xff;
        CU_FAIL("Additional page allocated unnecessarily"); 
    } else {
        CU_PASS("No additional memory allocated unnecessarily");
    }

    (void) signal(SIGSEGV, old_handler);
}

void
t_pool_destroy(void)
{
    /* Assuming this test is run AFTER the t_pool_create function */
    CU_ASSERT_NOT_EQUAL_FATAL(pool_0, NULL_POOL);

    void *p0_addr =  (void*) (pool_0 & ((( 1llu << 16 ) - 1) << 32));

    CU_ASSERT_EQUAL(pool_destroy(&pool_0), 0);
    CU_ASSERT_EQUAL(pool_0, NULL_POOL);

    void (*old_handler)(int) = signal(SIGSEGV, segv_handler);

    if (sigsetjmp(context, 1) == 0) {
        memset(p0_addr, 0xff, PAGE_SIZE);
        CU_FAIL("Deallocated memory wasn't reclaimed");
    } else {
        CU_PASS("Deallocated memory was reclaimed");
    }

    (void) signal(SIGSEGV, old_handler);
}

void
t_set_and_get_field_reference(void)
{
    pool_reference list_pool = pool_create(LIST_TYPE_ID);
    CU_ASSERT_NOT_EQUAL_FATAL(list_pool, NULL_POOL);

    global_reference ref_0 = pool_alloc(&list_pool);
    global_reference ref_1 = pool_alloc(&list_pool);
    global_reference ref_2 = pool_alloc(&list_pool);

    /* Reference two steps forward */
    CU_ASSERT_EQUAL(set_field_reference(ref_0, 0, ref_2), 0);
    CU_ASSERT_EQUAL(get_field_reference(ref_0, 0), ref_2);

    /* Reference two steps backwards */
    CU_ASSERT_EQUAL(set_field_reference(ref_2, 0, ref_0), 0);
    CU_ASSERT_EQUAL(get_field_reference(ref_2, 0), ref_0);

    pool_grow(&list_pool, 5000);
    global_reference remote_ref = pool_get_ref(list_pool, 5000);

    CU_ASSERT_EQUAL(set_field_reference(ref_1, 0, remote_ref), 0);
    CU_ASSERT_EQUAL(get_field_reference(ref_1, 0), remote_ref);


    pool_destroy(&list_pool);
}

