#include <stddef.h>
#include "stdlib.h"
#include "list.h"
#include "alloc.h"

void list_init(struct list* list)
{
    ASSERT(list, "Initializing null list");
    list->head.prev = NULL;
    list->head.next = NULL;
    list->tail.prev = NULL;
    list->tail.next = NULL;
}

/**
 * @brief Append an item to a list.
 *
 * @param list the list to append to
 * @param item the item to append to the list
 */
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

/**
 * @brief Create a list node from a pointer to some data.
 *
 * @param data the data to store in the list node
 * @return struct list_node* the list node representing the data
 */
struct list_node* list_node(void* data)
{
    struct list_node* node = kalloc(sizeof(*node));
    node->value = data;
    return node;
}

/**
 * @brief Remove an item from a list.
 *
 * @param item the item to remove
 */
void list_remove(struct list_node* item)
{
    struct list_node* prev = item->prev;
    struct list_node* next = item->next;

    prev->next = next;
    next->prev = prev;

    kfree(item);
}

/**
 * @brief Insert an item into a list.
 *
 * @param before the item to insert this item before
 * @param item the item to insert into the list
 */
void list_insert(struct list_node* before, struct list_node* item)
{
    struct list_node* oldnext = before->next;
    before->next = item;
    oldnext->prev = item;
}

/**
 * @brief Get the next item in a list after the given item.
 *
 * @param item the item to get the next item for
 * @return struct list_node* the next item
 */
struct list_node* list_next(struct list_node* item)
{
    return item->next;
}

/**
 * @brief Get the previous item in a list before the given item.
 *
 * @param item the item to get the previous item for
 * @return struct list_node* the previous item
 */
struct list_node* list_prev(struct list_node* item)
{
    return item->prev;
}

/**
 * @brief Iterate over a list, calling the consumer for each item
 *
 * @param list the list to iterate over
 * @param consumer the consumer to be called for each item in the list
 */
void list_iterate(struct list* list, list_consumer consumer)
{
    LIST_FOREACH(current, list) {
        consumer(current);
    }
}

/**
 * @brief Enumerate over a list, calling the enumerator for each item
 *
 * @param list the list to enumerate over
 * @param enumerator the enumerator to call for each item
 */
void list_enumerate(struct list* list, list_enumerator enumerator)
{
    int i = 0;
    LIST_FOREACH(current, list) {
        enumerator(i++, current);
    }
}

/**
 * @brief Get the head of a list.
 *
 * @param list the list to get the head of
 * @return struct list_node* the head of the list
 */
struct list_node* list_head(struct list* list)
{
    return &list->head;
}
