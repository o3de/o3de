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
        : m_indicesPrevious(AZStd::make_unique<T[]>(size))
        , m_indicesNext(AZStd::make_unique<T[]>(size))
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
            Remove<true, false>(index);

            // Reset indices.
            m_indicesPrevious[index] = m_back;
            m_indicesNext[index] = InvalidIndex;

            // Insert at the back.
            m_indicesNext[m_back] = index;
            m_back = index;
        }
    }

    template<typename T>
    void RecentlyUsedIndex<T>::Flush(T index)
    {
        if (index != m_front)
        {
            Remove<false, true>(index);

            // Reset indices.
            m_indicesPrevious[index] = InvalidIndex;
            m_indicesNext[index] = m_front;

            // Insert at the front.
            m_indicesPrevious[m_front] = index;
            m_front = index;
        }
    }

    template<typename T>
    void RecentlyUsedIndex<T>::FlushAll()
    {
        m_indicesPrevious[0] = InvalidIndex;
        for (T i = 1; i < m_size; ++i)
        {
            m_indicesPrevious[i] = i - 1;
        }

        for (T i = 0; i < m_size - 1; ++i)
        {
            m_indicesNext[i] = i + 1;
        }
        m_indicesNext[m_size - 1] = InvalidIndex;
    }

    template<typename T>
    T RecentlyUsedIndex<T>::TouchLeastRecentlyUsed()
    {
        AZ_Assert(m_size > 0, "Least recently used index is begin touched in Recently Used Index before being initialized with a size.");

        // Very similar to Touch and Flush, but some work and checks can be avoided as it's known that it's the front being moved to the back.
        T next = m_indicesNext[m_front];
        m_indicesNext[m_back] = m_front;
        m_indicesPrevious[next] = InvalidIndex;

        m_indicesPrevious[m_front] = m_back;
        m_indicesNext[m_front] = InvalidIndex;

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
            current = m_indicesNext[current];
        }
        AZ_Assert(current == InvalidIndex, "The double linked list in Recently Used Index has become corrupted.");
    }

    template<typename T>
    template<bool CheckFront, bool CheckBack>
    void RecentlyUsedIndex<T>::Remove(T index)
    {
        AZ_Assert(index < m_size, "Index out of bounds while calling Touch in Recently Used Index.");

        T previous = m_indicesPrevious[index];
        T next = m_indicesNext[index];
        if (previous != InvalidIndex)
        {
            m_indicesNext[previous] = next;
        }
        else
        {
            if constexpr (CheckFront)
            {
                m_front = next; // This index was at the front, so the front needs to be reset.
            }
        }

        if (next != InvalidIndex)
        {
            m_indicesPrevious[next] = previous;
        }
        else
        {
            if constexpr (CheckBack)
            {
                m_back = previous; // This index was at the back, so the back needs to be reset.
            }
        }
    }
} // namespace AZ::IO
