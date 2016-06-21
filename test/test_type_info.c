#include "test_type_info.h"


static const struct type_info ti_primitive_0 = {
    .type_id = CHAR_TYPE_ID,
    .type_class = PRIMITIVE_TYPE,
    .primitive_size = 1
};

static const struct type_info ti_primitive_1 = {
    .type_id = LONG_TYPE_ID,
    .type_class = PRIMITIVE_TYPE,
    .primitive_size = 8
};

static const struct type_info ti_global_ref_0 = {
    .type_id = CHAR_REF_TYPE_ID,
    .type_class = GLOBAL_REF_TYPE,
    .referee_type_id = CHAR_TYPE_ID 
};

static const struct composite_container_0 {
    const struct type_info ti_composite_0;
    Type_info        fields[4];
} container_0 = {
    .ti_composite_0 = { 
        .type_id = COMPOSITE_TYPE_1_ID ,
        .type_class = COMPOSITE_TYPE,
        .field_count= 4},
    .fields = { 
        &ti_primitive_0,
        &ti_primitive_0,
        &ti_primitive_0,
        &ti_primitive_1 }
};

static const struct composite_container_1 {
    const struct type_info ti_composite_1;
    Type_info        fields[3];
} container_1 = {
    .ti_composite_1 = { 
        .type_id = COMPOSITE_TYPE_2_ID,
        .type_class = COMPOSITE_TYPE,
        .field_count= 3},
    .fields = { 
        &ti_primitive_1,
        &container_0.ti_composite_0,
        &container_0.ti_composite_0 }
};

static const struct type_info ti_list_global_ref = {
    .type_id = LIST_GLOBAL_REF_TYPE_ID,
    .type_class = GLOBAL_REF_TYPE,
    .referee_type_id = LIST_TYPE_ID
};

static const struct type_info ti_list_local_ref = {
    .type_id = LIST_LOCAL_REF_TYPE_ID,
    .type_class = LOCAL_REF_TYPE,
    .referee_type_id = LIST_TYPE_ID
};

static const struct list_container {
    const struct type_info ti_list;
    Type_info    fields[3];
} list_container = {
    .ti_list = {
        .type_id = LIST_TYPE_ID,
        .type_class = COMPOSITE_TYPE,
        .field_count = 3 },
    .fields = {
        &ti_list_local_ref,
        &ti_primitive_1,
        &ti_primitive_1,
    }
};

static const struct type_info ti_btree_local_ref = {
    .type_id = BTREE_LOCAL_REF_TYPE_ID,
    .type_class = LOCAL_REF_TYPE,
    .referee_type_id = BTREE_TYPE_ID
};

static const struct btree_container {
    const struct type_info ti_btree;
    Type_info    fields[4];
} btree_container = {
    .ti_btree = {
        .type_id = BTREE_TYPE_ID,
        .type_class = COMPOSITE_TYPE,
        .field_count = 4 },
    .fields = {
        &ti_btree_local_ref,
        &ti_btree_local_ref,
	&ti_primitive_1,
        &ti_primitive_1 }
};

static const struct type_info ti_otree_local_ref = {
    .type_id = OTREE_LOCAL_REF_TYPE_ID,
    .type_class = LOCAL_REF_TYPE,
    .referee_type_id = OTREE_TYPE_ID
};

static const struct otree_container {
    const struct type_info ti_otree;
    Type_info fields[10];
} otree_container = {
    .ti_otree = {
        .type_id = OTREE_TYPE_ID,
        .type_class = COMPOSITE_TYPE,
        .field_count = 10 },
    .fields = {
        &ti_otree_local_ref,
        &ti_otree_local_ref,
        &ti_otree_local_ref,
        &ti_otree_local_ref,
        &ti_otree_local_ref,
        &ti_otree_local_ref,
        &ti_otree_local_ref,
        &ti_otree_local_ref,
        &ti_primitive_1, 
        &ti_primitive_1 }
};

static const struct type_info ti_reference_table_entry = {
    .type_id = REFERENCE_TABLE_ENTRY,
    .type_class = PRIMITIVE_TYPE,
    .primitive_size = 16
};

void
t_get_size_and_field_count(void)
{
    /* Test of simple primitive data types */


    /* First a primitive char */
    size_t ti_primitive_0_size;
    size_t ti_primitive_0_field_count;

    get_size_and_field_count(&ti_primitive_0, 
                             &ti_primitive_0_size, 
                             &ti_primitive_0_field_count);

    CU_ASSERT_EQUAL(ti_primitive_0_size, 1u);
    CU_ASSERT_EQUAL(ti_primitive_0_field_count, 1u);

    /* Primitive double or long long */
    size_t ti_primitive_1_size;
    size_t ti_primitive_1_field_count;

    get_size_and_field_count(&ti_primitive_1, 
                             &ti_primitive_1_size, 
                             &ti_primitive_1_field_count);

    CU_ASSERT_EQUAL(ti_primitive_1_size, 8u);
    CU_ASSERT_EQUAL(ti_primitive_1_field_count, 1u);

    /* Primitive reference (global) */
    size_t ti_global_ref_0_size;
    size_t ti_global_ref_0_field_count;

    get_size_and_field_count(&ti_global_ref_0,
                             &ti_global_ref_0_size,
                             &ti_global_ref_0_field_count);

    CU_ASSERT_EQUAL(ti_global_ref_0_size, 8u);
    CU_ASSERT_EQUAL(ti_global_ref_0_field_count, 1u);

    /* Test of composite, un-nested data type */
    size_t ti_composite_0_size;
    size_t ti_composite_0_field_count;

    get_size_and_field_count(&container_0.ti_composite_0, 
                             &ti_composite_0_size, 
                             &ti_composite_0_field_count);

    CU_ASSERT_EQUAL(ti_composite_0_size, 11u);
    CU_ASSERT_EQUAL(ti_composite_0_field_count, 4u);

    /* Test of composite, nested data type */

    size_t ti_composite_1_size;
    size_t ti_composite_1_field_count;
    get_size_and_field_count(&container_1.ti_composite_1, 
                             &ti_composite_1_size, 
                             &ti_composite_1_field_count);

    CU_ASSERT_EQUAL(ti_composite_1_size, 30u);
    CU_ASSERT_EQUAL(ti_composite_1_field_count, 9u);
}

void
t_fill_in_offsets(void)
{
    /* First test a simple primitive type, a char */
    size_t base_offset = 0u;
    struct field_offset fo_primitive_0 = {0xDEAD, 0xDEADBEEF, 0xBABEFACE};
    (void) fill_in_offsets(&fo_primitive_0,
			   &ti_primitive_0,
			   &base_offset);

    CU_ASSERT_EQUAL(base_offset, 1u);
    CU_ASSERT_EQUAL(fo_primitive_0.field_size, 1u);
    CU_ASSERT_EQUAL(fo_primitive_0.offset, 0u);

    /* Test of another primitive (reference) type */
    base_offset = 10u;
    struct field_offset fo_global_ref_0 = {0xDEAD, 0xDEADBEEF, 0xBABEFACE};
    (void) fill_in_offsets(&fo_global_ref_0,
			   &ti_global_ref_0,
			   &base_offset);

    CU_ASSERT_EQUAL(base_offset, 10u + 8u);
    CU_ASSERT_EQUAL(fo_global_ref_0.field_size, 8u);
    CU_ASSERT_EQUAL(fo_global_ref_0.offset, 10u);

    /* Test of a non nestled, composite type */
    base_offset = 0u;
    struct field_offset fo_composite_0[4];
    (void) fill_in_offsets(fo_composite_0,
			   &container_0.ti_composite_0,
			   &base_offset);

    CU_ASSERT_EQUAL(base_offset, 11u);
    CU_ASSERT_EQUAL(fo_composite_0[0].field_size, 1);
    CU_ASSERT_EQUAL(fo_composite_0[0].offset, 0);
    CU_ASSERT_EQUAL(fo_composite_0[1].field_size, 1);
    CU_ASSERT_EQUAL(fo_composite_0[1].offset, 1);
    CU_ASSERT_EQUAL(fo_composite_0[2].field_size, 1);
    CU_ASSERT_EQUAL(fo_composite_0[2].offset, 2);
    CU_ASSERT_EQUAL(fo_composite_0[3].field_size, 8);
    CU_ASSERT_EQUAL(fo_composite_0[3].offset, 3);

    /* Test of a nestled, composite type */
    base_offset = 0u;
    struct field_offset fo_composite_1[9];
    (void) fill_in_offsets(fo_composite_1,
			   &container_1.ti_composite_1,
			   &base_offset);

    CU_ASSERT_EQUAL(base_offset, 30u);

    /* The final representation of a nestled type is "flattened" */
    /* The first, primitive field */
    CU_ASSERT_EQUAL(fo_composite_1[0].field_size, 8);
    CU_ASSERT_EQUAL(fo_composite_1[0].offset, 0);

    /* The fields belonging to the first nestled composite type */
    CU_ASSERT_EQUAL(fo_composite_1[1].field_size, 1);
    CU_ASSERT_EQUAL(fo_composite_1[1].offset, 8);
    CU_ASSERT_EQUAL(fo_composite_1[2].field_size, 1);
    CU_ASSERT_EQUAL(fo_composite_1[2].offset, 9);
    CU_ASSERT_EQUAL(fo_composite_1[3].field_size, 1);
    CU_ASSERT_EQUAL(fo_composite_1[3].offset, 10);
    CU_ASSERT_EQUAL(fo_composite_1[4].field_size, 8);
    CU_ASSERT_EQUAL(fo_composite_1[4].offset, 11);

    /* The fields belonging to the second nestled composite type */
    CU_ASSERT_EQUAL(fo_composite_1[5].field_size, 1);
    CU_ASSERT_EQUAL(fo_composite_1[5].offset, 19);
    CU_ASSERT_EQUAL(fo_composite_1[6].field_size, 1);
    CU_ASSERT_EQUAL(fo_composite_1[6].offset, 20);
    CU_ASSERT_EQUAL(fo_composite_1[7].field_size, 1);
    CU_ASSERT_EQUAL(fo_composite_1[7].offset, 21);
    CU_ASSERT_EQUAL(fo_composite_1[8].field_size, 8);
    CU_ASSERT_EQUAL(fo_composite_1[8].offset, 22);
}

int 
add_basic_types(void) {
    Type_info type_infos[] = {
        &ti_primitive_0,
        &ti_primitive_1,
        &ti_global_ref_0,
        &container_0.ti_composite_0,
        &container_1.ti_composite_1,
        &ti_list_global_ref,
        &ti_list_local_ref,
        &list_container.ti_list,
        &ti_btree_local_ref,
        &btree_container.ti_btree,
        &ti_otree_local_ref,
        &otree_container.ti_otree,
        &ti_reference_table_entry
    };

    return init_type_table(sizeof(type_infos) / sizeof(void*),
			               type_infos);
}

void
t_init_type_table(void)
{
    int init_result = add_basic_types();

    CU_ASSERT_EQUAL(init_result, 0);

    extern Type_table type_table;
    
    CU_ASSERT_EQUAL(type_table[0].type_size, 1u);
    CU_ASSERT_EQUAL(type_table[0].field_count, 1u);

    CU_ASSERT_EQUAL(type_table[1].type_size, 8u);
    CU_ASSERT_EQUAL(type_table[1].field_count, 1u);

    CU_ASSERT_EQUAL(type_table[2].type_size, 8u);
    CU_ASSERT_EQUAL(type_table[2].field_count, 1u);

    CU_ASSERT_EQUAL(type_table[3].type_size, 11u);
    CU_ASSERT_EQUAL(type_table[3].field_count, 4u);

    CU_ASSERT_EQUAL(type_table[4].type_size, 30u);
    CU_ASSERT_EQUAL(type_table[4].field_count, 9u);


    CU_ASSERT_EQUAL(type_table[0].field_offsets->field_size, 1);
    CU_ASSERT_EQUAL(type_table[0].field_offsets->offset, 0);

    CU_ASSERT_EQUAL(type_table[1].field_offsets->field_size, 8);
    CU_ASSERT_EQUAL(type_table[1].field_offsets->offset, 0);

    CU_ASSERT_EQUAL(type_table[2].field_offsets->field_size, 8);
    CU_ASSERT_EQUAL(type_table[2].field_offsets->offset, 0);

    CU_ASSERT_EQUAL(type_table[3].field_offsets[0].field_size, 1);
    CU_ASSERT_EQUAL(type_table[3].field_offsets[1].field_size, 1);
    CU_ASSERT_EQUAL(type_table[3].field_offsets[2].field_size, 1);
    CU_ASSERT_EQUAL(type_table[3].field_offsets[3].field_size, 8);
    CU_ASSERT_EQUAL(type_table[3].field_offsets[0].offset, 0);
    CU_ASSERT_EQUAL(type_table[3].field_offsets[1].offset, 1);
    CU_ASSERT_EQUAL(type_table[3].field_offsets[2].offset, 2);
    CU_ASSERT_EQUAL(type_table[3].field_offsets[3].offset, 3);

    CU_ASSERT_EQUAL(type_table[4].field_offsets[0].field_size, 8);

    CU_ASSERT_EQUAL(type_table[4].field_offsets[1].field_size, 1);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[2].field_size, 1);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[3].field_size, 1);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[4].field_size, 8);

    CU_ASSERT_EQUAL(type_table[4].field_offsets[5].field_size, 1);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[6].field_size, 1);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[7].field_size, 1);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[8].field_size, 8);

    CU_ASSERT_EQUAL(type_table[4].field_offsets[0].offset, 0);

    CU_ASSERT_EQUAL(type_table[4].field_offsets[1].offset, 8);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[2].offset, 9);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[3].offset, 10);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[4].offset, 11);

    CU_ASSERT_EQUAL(type_table[4].field_offsets[5].offset, 19);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[6].offset, 20);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[7].offset, 21);
    CU_ASSERT_EQUAL(type_table[4].field_offsets[8].offset, 22);
}

