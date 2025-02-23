#ifndef KERNEL_LIST_H
#define KERNEL_LIST_H

#include <stddef.h>

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *head) {
    head->next = head;
    head->prev = head;
}

static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head) {
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head) {
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
}

static inline int list_empty(const struct list_head *head) {
    return head->next == head;
}

#define container_of(ptr, type, member) container_of_func(ptr, (type *)0, offsetof(type, member))

static inline void *container_of_func(const void *ptr, const void *dummy, size_t offset) {
    (void)dummy;
    return (void *)((char *)ptr - offset);
}

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, type, head, member)              \
    for (pos = list_entry((head)->next, type, member);  \
         &pos->member != (head);                    \
         pos = list_entry(pos->member.next, type, member))

#define list_for_each_entry_safe(pos, type, n, head, member)          \
    for (pos = list_entry((head)->next, type, member),  \
        n = list_entry(pos->member.next, type, member); \
            &pos->member != (head);                    \
            pos = n, n = list_entry(n->member.next, type, member))

#endif /* KERNEL_LIST_H */
