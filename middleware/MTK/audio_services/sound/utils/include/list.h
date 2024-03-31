#ifndef _LIST_H
#define _LIST_H

#include <stddef.h>

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};
#define LIST_HEAD_INIT(name) {&(name), &(name)}

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name);

/* initialize a list head explicitly */
static inline void INIT_LIST_HEAD(struct list_head *p)
{
    p->next = p->prev = p;
}

#define list_entry_offset(p, type, offset) \
    ((type *)((char *)(p) - (offset)))

/* list_entry - retrieve the original struct from list_head
 * @p: list_head pointer
 * @type: struct type
 * @member: struct field member containing the list_head
 */
#define list_entry(p, type, member) \
    list_entry_offset(p, type, offsetof(type, member))

/* list_for_each - iterate over the linked list
 * @p: iterator, a list_head pointer variable
 * @list: list_head pointer containing the list
 */
#define list_for_each(p, list) \
    for (p = (list)->next; p != (list); p = p->next)

/* list_for_each_safe - iterate over the linked list, safe to delete
 * @p: iterator, a list_head pointer variable
 * @s: a temporary variable to keep the next, a list_head pointer, too
 * @list: list_head pointer containing the list
 */
#define list_for_each_safe(p, s, list) \
    for (p = (list)->next; s = p->next, p != (list); p = s)

/* list_add - prepend a list entry at the head
* @p: the new list entry to add
* @list: the list head
*/
static inline void list_add(struct list_head *p, struct list_head *list)
{
    struct list_head *first = list->next;

    p->next = first;
    first->prev = p;
    list->next = p;
    p->prev = list;
}

/* list_add_tail - append a list entry at the tail
 * @p: the new list entry to add
 * @list: the list head
 */
static inline void list_add_tail(struct list_head *p, struct list_head *list)
{
    struct list_head *last = list->prev;

    last->next = p;
    p->prev = last;
    p->next = list;
    list->prev = p;
}

/* list_del - delete the given list entry */
static inline void list_del(struct list_head *p)
{
    p->prev->next = p->next;
    p->next->prev = p->prev;
}

/* list_empty - returns 1 if the given list is empty */
static inline int list_empty(const struct list_head *p)
{
    return p->next == p;
}

/** * list_first_entry - get the first element from a list
 * @ptr:    the list head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

/** * list_next_entry - get the next element in list
 * @pos:    the type * to cursor
 * @member: the name of the list_head within the struct.
 */
#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)

/** * list_for_each_entry   -   iterate over list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your list.
 * @member: the name of the list_head within the struct.
 */
#define list_for_each_entry(pos, head, member)              \
    for (pos = list_first_entry(head, typeof(*pos), member);    \
        &pos->member != (head);                 \
        pos = list_next_entry(pos, member))

/** * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:    the type * to use as a loop cursor.
 * @n:      another type * to use as temporary storage
 * @head:   the head for your list.
 * @member: the name of the list_head within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)          \
    for (pos = list_first_entry(head, typeof(*pos), member),    \
         n = list_next_entry(pos, member);          \
         &pos->member != (head);                    \
         pos = n, n = list_next_entry(n, member))

#endif /* #ifndef _LIST_H */
