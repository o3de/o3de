/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ::IO
{
    template<typename T>
    RecentlyUsedIndex<T>::RecentlyUsedIndex(T size)
        : m_indices_previous(AZStd::make_unique<T[]>(size))
        , m_indices_next(AZStd::make_unique<T[]>(size))
        , m_size(size)
        , m_front(0)
        , m_back(size - 1)
    {
        AZ_Assert(size > 2, "At least 2 entries are needed in order to have a separate front and back.");
        AZ_Assert(size < AZStd::numeric_limits<T>::max() - 1, "Size too large for Recently Used Index with the provided type.");

        FlushAll();
    }

    template<typename T>
    void RecentlyUsedIndex<T>::Touch(T index)
    {
        if (index != m_back)
        {
            AZ_Assert(index < m_size, "Index out of bounds while calling Touch in Recently Used Index.");

            // Remove the index from its current spot and fix m_front if needed.
            T previous = m_indices_previous[index];
            T next = m_indices_next[index];
            if (previous != InvalidIndex)
            {
                m_indices_next[previous] = next;
            }
            else
            {
                m_front = next; // This index was at the front, so the front needs to be reset.
            }

            if (next != InvalidIndex)
            {
                m_indices_previous[next] = previous;
            }

            // Reset indices.
            m_indices_previous[index] = m_back;
            m_indices_next[index] = InvalidIndex;

            // Insert at the back.
            m_indices_next[m_back] = index;
            m_back = index;
        }
    }

    template<typename T>
    void RecentlyUsedIndex<T>::Flush(T index)
    {
        if (index != m_front)
        {
            AZ_Assert(index < m_size, "Index out of bounds while calling Flush in Recently Used Index.");

            T previous = m_indices_previous[index];
            T next = m_indices_next[index];
            if (previous != InvalidIndex)
            {
                m_indices_next[previous] = next;
            }

            if (next != InvalidIndex)
            {
                m_indices_previous[next] = previous;
            }
            else
            {
                m_back = previous; // This index was at the front, so the front needs to be reset.
            }

            // Reset indices.
            m_indices_previous[index] = InvalidIndex;
            m_indices_next[index] = m_front;

            // Insert at the front.
            m_indices_previous[m_front] = index;
            m_front = index;
        }
    }

    template<typename T>
    void RecentlyUsedIndex<T>::FlushAll()
    {
        m_indices_previous[0] = InvalidIndex;
        for (T i = 1; i < m_size; ++i)
        {
            m_indices_previous[i] = i - 1;
        }

        for (T i = 0; i < m_size - 1; ++i)
        {
            m_indices_next[i] = i + 1;
        }
        m_indices_next[m_size - 1] = InvalidIndex;
    }

    template<typename T>
    T RecentlyUsedIndex<T>::TouchLeastRecentlyUsed()
    {
        AZ_Assert(m_size > 0, "Least recently used index is begin touched in Recently Used Index before being initialized with a size.");

        // Very similar to Touch and Flush, but some work and checks can be avoided as it's known that it's the front being moved to the back.
        T next = m_indices_next[m_front];
        m_indices_next[m_back] = m_front;
        m_indices_previous[next] = InvalidIndex;

        m_indices_previous[m_front] = m_back;
        m_indices_next[m_front] = InvalidIndex;

        m_back = m_front;
        m_front = next;

        return m_back;
    }

    template<typename T>
    T RecentlyUsedIndex<T>::GetLeastRecentlyUsed() const
    {
        return m_front;
    }

    template<typename T>
    T RecentlyUsedIndex<T>::GetMostRecentlyUsed() const
    {
        return m_back;
    }

    template<typename T>
    void RecentlyUsedIndex<T>::GetIndicesInOrder(const AZStd::function<void(T)>& callback) const
    {
        T current = m_front;
        for (T i = 0; i < m_size; ++i)
        {
            callback(current);
            current = m_indices_next[current];
        }
        AZ_Assert(current == InvalidIndex, "The double linked list in Recently Used Index has become corrupted.");
    }
} // namespace AZ::IO
