/**
 * @brief Defines a set of common types that will always be present when using
 * local heaps.
 *
 * The types id:s declared here are used in many test cases, as well as in the
 * microbenchmarks. They are defined in the test_type_info.c testcase, it is
 * recommended that a set of useful types are identified and placed in a more
 * logical place before this system is put into production use.
 *
 * @file basic_types.h
 * @author Martin Hagelin
 * @date Decmber, 2014
 *
 */

#ifndef __BASIC_TYPES_H__
#define __BASIC_TYPES_H__

typedef enum type_ids {
    CHAR_TYPE_ID = 0,
    LONG_TYPE_ID = 1,
    CHAR_REF_TYPE_ID = 2,
    COMPOSITE_TYPE_1_ID = 3,
    COMPOSITE_TYPE_2_ID = 4,
    LIST_GLOBAL_REF_TYPE_ID = 5,
    LIST_LOCAL_REF_TYPE_ID = 6,
    LIST_TYPE_ID = 7,
    BTREE_LOCAL_REF_TYPE_ID = 8,
    BTREE_TYPE_ID = 9,
    OTREE_LOCAL_REF_TYPE_ID = 10,
    OTREE_TYPE_ID = 11,
    REFERENCE_TABLE_ENTRY = 12
} TYPE_ID;

#endif
