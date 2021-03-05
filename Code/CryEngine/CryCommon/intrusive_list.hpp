/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef _INTRUSIVE_LIST_HPP
#define _INTRUSIVE_LIST_HPP
#if !defined(WIN32)
#include <stdint.h>
#endif //!defined(WIN32)


namespace util
{
    // Simple lightweight intrusive list utility
    //
    template<typename Host>
    struct list
    {
        typedef Host value_type;

        list* next;
        list* prev;

        // Default (initializing) constructor
        list()
        {
            next = this;
            prev = this;
        }

        // Inserts this list into a given list item between
        // prev and next (note: they need to be sequential!)
        void insert(list* _prev, list* _next)
        {
            _next->prev = this;
            this->next = _next;
            this->prev = _prev;
            _prev->next = this;
        }

        // Should only be called on heads
        void clear() { next = prev = this;  }

        // Removes from a list, then inserts this list item into a given list between
        // prev and next (note: they need to be sequential!)
        void re_insert(list* _prev, list* _next)
        {
            erase();
            insert(_prev, _next);
        }


        // Remove this list instance from a list
        // Safe to be called even if not attached to a list
        void erase()
        {
            next->prev = prev;
            prev->next = next;
            next = prev = this;
        }

        // Predicate to determine if a list structure is linked to a list
        bool empty() const { return prev == this && next == this; }
        bool linked() const { return !empty(); }

        // Get the host instance of the instrusive list
        template<list Host::* member>
        Host* item()
        {
            return ((Host*)((uintptr_t)(this) - (uintptr_t)(&(((Host*)0)->*member))));
        }
        template<list Host::* member>
        const Host* item() const
        {
            return ((Host*)((uintptr_t)(this) - (uintptr_t)(&(((Host*)0)->*member))));
        }
        template<list (Host::* Member)[2]>
        Host * item(int threadId)
        {
            uintptr_t value = (uintptr_t)this - (uintptr_t)&((((Host*)(NULL))->*Member)[threadId]);
            return alias_cast<Host*>(value);
        }

        // Insert & relink functions
        void insert_tail(list* _list) { insert(_list->prev, _list); }
        void insert_tail(list& _list) { insert_tail(&_list); }
        void insert_head(list* _list) { insert(_list, _list->next); }
        void insert_head(list& _list) { insert_head(&_list); }
        void relink_tail(list* _list) { erase(); insert_tail(_list); }
        void relink_tail(list& _list) { relink_tail(&_list); }
        void relink_head(list* _list) { erase(); insert_head(_list); }
        void relink_head(list& _list) { relink_head(&_list); }

        static inline void splice(const list* _list, list* prev, list* next)
        {
            list* first = _list->next;
            list* last = _list->prev;

            first->prev = prev;
            prev->next = first;

            last->next = next;
            next->prev = last;
        }

        void splice_front(const list* _list)
        {
            if (!_list->empty())
            {
                splice(_list, this, this->next);
                _list->clear();
            }
        }

        void splice_tail(list* _list)
        {
            if (!_list->empty())
            {
                splice(_list, this->prev, this);
                _list->clear();
            }
        }

        // Accessors
        list<Host>* tail() { return prev; }
        const list<Host>* tail() const { return prev; }
        list<Host>* head() { return head; }
        const list<Host>* head() const { return head; }
    };

    // utility to extract the host type of an intrusive list at compile time
    template<typename List>
    struct list_value_type
    {
        typedef typename List::value_type value_type;
    };

    // Loops over the list in forward manner, pos will be of type list<T>*
# define list_for_each(pos, head)                       \
    for (typeof(head)pos = (head)->next; pos != (head); \
         pos = pos->next)

    // Loops over the list in backward manner, pos will be of type list<T>*
# define list_for_each_backwards(pos, head)             \
    for (typeof(head)pos = (head)->prev; pos != (head); \
         pos = pos->prev)

    // Loops over the list in forward manner while safeguarding
    // against list removal during iteration. pos will be of type list<T>*
# define list_for_each_safe(pos, head)                                 \
    for (typeof(head)pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

    // Loops over the list in forward manner. pos will be of type list<T>*
# define list_for_each_entry(pos, head, member)                   \
    for (util::list_value_type<typeof((head))>::value_type* pos = \
             (head)->next->item<member>();                        \
         &(pos->*(member)) != &list;                              \
         pos = ((pos->*(member)).next->item<member>()))

    // Loops over the list in forward manner while safeguarding against list
    // removal during iteratation. pos will be of type T*
# define list_for_each_entry_safe(pos, head, member)              \
    for (util::list_value_type<typeof((head))>::value_type* pos = \
             (head)->next->item<member>(),                        \
         * n = ((pos->*(member)).next->item<member>());           \
         &(pos->*(member)) != &list;                              \
         pos = n, n = ((n->*(member)).next->item<member>()))
} // end namespace util
#endif // ifndef _INTRUSIVE_LIST_HPP

