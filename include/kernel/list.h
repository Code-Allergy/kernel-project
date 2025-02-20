#ifndef KERNEL_LIST_H
#define KERNEL_LIST_H

#include <stddef.h> // for offsetof

struct list_node {
    struct list_node *next;
    struct list_node *prev;
};

// Initialize a list head
#define LIST_INIT(head) { (head)->next = (head); (head)->prev = (head); }

// Add a new node between two existing nodes
#define LIST_ADD(new, prev, next) do { \
    (next)->prev = (new); \
    (new)->next = (next); \
    (new)->prev = (prev); \
    (prev)->next = (new); \
} while (0)

// Remove a node by updating its neighbors
#define LIST_DEL(entry) do { \
    (entry)->prev->next = (entry)->next; \
    (entry)->next->prev = (entry)->prev; \
} while (0)

// Check if the list is empty
#define LIST_IS_EMPTY(head) ((head)->next == (head))

// Get the containing struct from a list_node pointer
#define LIST_ENTRY(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

// Iterate over list nodes
#define LIST_FOR_EACH(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

// Iterate safely over list nodes (allows deletion during iteration)
#define LIST_FOR_EACH_SAFE(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

// Iterate over entries in the list
#define LIST_FOR_EACH_ENTRY(pos, head, member) \
    for (pos = LIST_ENTRY((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = LIST_ENTRY(pos->member.next, typeof(*pos), member))

#endif // LIST_H
