#include <stddef.h>
#include "stdlib.h"
#include "list.h"
#include "alloc.h"

void list_init(struct list* list)
{
    list->head.prev = NULL;
    list->head.next = NULL;
    list->tail.prev = NULL;
    list->tail.next = NULL;
}

void list_append(struct list* list, struct list_node* item)
{
    if (list->tail.prev) {
        struct list_node* prev = list->tail.prev;
        list->tail.prev = item;
        prev->next = item;

        item->prev = prev;
        item->next = &list->tail;
    } else {
        // Empty list scenario
        ASSERT(!list->head.next, "Expected empty list");
        list->tail.prev = item;
        list->head.next = item;
        item->prev = &list->head;
        item->next = &list->tail;
    }
}

struct list_node* list_node(void* data)
{
    struct list_node* node = kalloc(sizeof(*node));
    node->value = data;
    return node;
}

void list_remove(struct list* list, struct list_node* item)
{

}

void list_insert(struct list* list, struct list_node* item)
{

}

struct list_node* list_next(struct list_node* item)
{
    return item->next;
}

struct list_node* list_prev(struct list_node* item)
{
    return item->prev;
}

void list_iterate(struct list* list, list_consumer consumer)
{
    struct list_node* current = &list->head;
    while ((current = list_next(current)) && current->next) {
        consumer(current);
    }
}

void list_enumerate(struct list* list, list_enumerator enumerator)
{
    struct list_node* current = &list->head;
    int i = 0;
    while ((current = list_next(current))->next) {
        enumerator(i++, current);
    }
}