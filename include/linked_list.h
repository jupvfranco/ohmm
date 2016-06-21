/** 
 * @brief A simple linked list to use as a baseline.
 *
 *
 * @file linked_list.c
 * @author Martin Hagelin
 * @date January, 2015
 */



#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

#include <stdlib.h>
#include <stdint.h>

typedef struct node *Node;
struct node {
    Node        next;
    uint64_t    a;
    uint64_t    b;
};

Node 
new_node(Node list);

Node
new_unrelated(Node list, double bloat_variance);

void
destroy_list(Node list);

/**
 * @brief A less general map for fields in ordinary lists. 
 *
 */
uint64_t*
list_field_map(Node src,
               size_t length,
               void (*f)(void *, void*)); 


#endif
