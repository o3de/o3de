/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace Render
    {
        //! This container class implements a doubly linked list using a vector as the underlying data type. This allows indexing via the [] operator.
        //! Insertion or removal of items will not invalidate the indices of existing items
        //! Insertion and removal times are O(1), reserve is O(n)
        //! See Intro to Algorithms, section 10.3: A single array representation of objects
        template<typename T>
        class IndexableList
        {
        public:

            void reserve(size_t newCapacity)
            {
                m_data.reserve(newCapacity);
            }

            size_t size() const
            {
                return m_size;
            }

            size_t capacity() const
            {
                return m_data.capacity();
            }

            T& operator[](const size_t position)
            {
                return m_data[position].m_value;
            }

            const T& operator[](const size_t position) const
            {
                return m_data[position].m_value;
            }

            //! Returns the index of the head of the linked list. -1 if the list is empty
            int begin() const
            {
                return m_usedListHead;
            }

            //! Given the index of an item in use, returns the index of the next item in the linked list. -1 if the end of the linked list has been reached.
            int next(const int position) const
            {
                AZ_Assert(position >= 0 && position < capacity(), "next() called with invalid position.");
                return m_data[position].m_next;
            }

            //! Given the index of an item in use, returns the index of the previous item in the linked list. -1 if the end of the linked list has been reached.
            int prev(const int position)const
            {
                AZ_Assert(position >= 0 && position < capacity(), "prev() called with invalid position.");
                return m_data[position].m_prev;
            }

            //! Adds a new item to the linked list and returns the index of where this item was placed.
            int push_front(const T& value)
            {
                if (HasFreeSpaceLeft() == false)
                {
                    AddEmptyNode();
                }

                int placeToInsert = m_freeListHead;
                m_data[placeToInsert].m_value = value;
                m_freeListHead = m_data[placeToInsert].m_next;
                m_data[placeToInsert].m_next = m_usedListHead;
                m_data[placeToInsert].m_prev = -1;
                if (m_usedListHead != -1)
                {
                    m_data[m_usedListHead].m_prev = placeToInsert;
                }

                m_usedListHead = placeToInsert;
                m_size++;
                return placeToInsert;
            }

            //! Removes an existing item from the list.
            void erase(const int positionToRemove)
            {
                AZ_Assert(positionToRemove >= 0 && positionToRemove < capacity(), "Remove called with invalid position.");

                m_size--;

                const bool areDeletingListHead = (m_usedListHead == positionToRemove);
                if (areDeletingListHead)
                {
                    m_usedListHead = m_data[positionToRemove].m_next;
                }

                const int next = m_data[positionToRemove].m_next;
                const int prev = m_data[positionToRemove].m_prev;

                if (next != -1)
                {
                    m_data[next].m_prev = prev;
                }

                if (prev != -1)
                {
                    m_data[prev].m_next = next;
                }

                m_data[positionToRemove].m_next = m_freeListHead;
                m_freeListHead = positionToRemove;
            }

            void clear()
            {
                m_data.clear();
                m_freeListHead = -1;
                m_usedListHead = -1;
                m_size = 0;
            }

            // returns the size of the internal array that the linked list is built upon
            size_t array_size() const
            {
                return m_data.size();
            }

        private:

            bool HasFreeSpaceLeft() const
            {
                return m_freeListHead != -1;
            }

            void AddEmptyNode()
            {
                Node node;
                node.m_next = m_freeListHead;
                node.m_prev = -1;
                m_data.push_back(node);
                m_freeListHead = aznumeric_cast<int>(m_data.size() - 1);
            }

            struct Node
            {
                T m_value;
                int m_next = -1;
                int m_prev = -1;
            };

            AZStd::vector<Node> m_data;
            int m_freeListHead = -1;
            int m_usedListHead = -1;
            size_t m_size = 0;
        };
    }
}
