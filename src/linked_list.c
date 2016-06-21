#include "linked_list.h"


Node 
new_node(Node list)
{
    Node new = malloc(sizeof(struct node));

    if (new == NULL)
        return NULL;

    new->next = list;

    return new;
}

Node
new_unrelated(Node list, double bloat_factor)
{
    size_t bloat = bloat_factor * ( random() % sizeof(struct node));

    Node new = malloc(sizeof(struct node) + bloat);
    if (new == NULL)
        return NULL;

    new->next = list;

    return new;
}

void
destroy_list(Node list)
{
    while (list != NULL) {
        Node tmp = list->next;
        free(list);
        list = tmp;
    }
}

uint64_t*
list_field_map(Node src,
               size_t length,
               void (*f)(void *, void*))
{
    uint64_t *result = malloc(sizeof(uint64_t) * length);
    if (result == NULL)
        return NULL;

    for (uint64_t *b = result ; src != NULL ; src = src->next, b++) {
        f(&src->a, b);
    }
        
    return result;
}

