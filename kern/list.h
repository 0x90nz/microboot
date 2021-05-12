#pragma once

struct list_node {
    struct list_node* next;
    struct list_node* prev;
    void* value;
};

struct list {
    struct list_node head;
    struct list_node tail;
};

typedef void (*list_consumer)(struct list_node*);
typedef void (*list_enumerator)(int, struct list_node*);

void list_init(struct list* list);
void list_append(struct list* list, struct list_node* item);
void list_remove(struct list_node* item);
void list_insert(struct list_node* before, struct list_node* item);
void list_iterate(struct list* list, list_consumer consumer);
void list_enumerate(struct list* list, list_enumerator enumerator);
struct list_node* list_prev(struct list_node* item);
struct list_node* list_next(struct list_node* item);
struct list_node* list_head(struct list* list);
struct list_node* list_tail(struct list* list);
struct list_node* list_node(void* data);
void* list_value(struct list_node* item);

/**
 * @brief Iterate over a list.
 *
 * i is the current list item and l is the list.
 */
#define LIST_FOREACH(i, l) \
 \
for (struct list_node* i = list_head(l); i && list_next(i); i = list_next(i))
