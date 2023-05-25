/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

namespace AZ::Render
{
    //! SparseVector stores elements in a vector under the hood, but allows for empty slots in the
    //! vector. This means that elements can be added or removed without affecting the location of
    //! other elements. When a new element is reserved in the SparseVector, it will use a free slot
    //! if available. Otherwise, it will push the data onto the end of the vector.
    //!
    //! This class works by storing a linked list of elements in the empty slot, as well as an index
    //! to the first empty slot. Since the linked list uses size_t for its type, this class must be
    //! templated to a class that is at least sizeof(size_t).
    
    template<typename T>
    class SparseVector
    {
    public:

        // Elements must be at least as large as size_t because empty slots are used to hold the index of the next
        // empty slot, which is a size_t. In the future this could be relaxed with an additional template argument
        // that would control the index type and therefor the maximum size of the SparseVector.
        static_assert(sizeof(T) >= sizeof(size_t), "Data stored in SparseVector must be at least as large as a size_t.");

        SparseVector();
        ~SparseVector();

        //! Reserves an element in the underlying vector and returns the index to that element.
        size_t Reserve();

        //! Releases an element with the given index so it can be reused.
        void Release(size_t index);

        //! Clears all the data from the underlying vector and resets the size to 0
        void Clear();

        //! Returns the size of the underlying vector. This is not the same as the number of
        //! valid elements in the vector since there can be empty slots.
        size_t GetSize() const;

        //! Returns a reference to the element at a given index
        T& GetElement(size_t index);

        //! Returns a pointer to the raw data vector.
        const T* GetRawData() const;

    private:

        static constexpr size_t NoFreeSlot = std::numeric_limits<size_t>::max();
        static constexpr size_t InitialReservedCount = 128;

        size_t m_nextFreeSlot = NoFreeSlot;
        AZStd::vector<T> m_data;
    };
    
    template<typename T>
    SparseVector<T>::SparseVector()
    {
        m_data.reserve(InitialReservedCount);
    }

    template<typename T>
    SparseVector<T>::~SparseVector()
    {
        Clear();
    }

    template<typename T>
    inline size_t SparseVector<T>::Reserve()
    {
        size_t slotToReturn = std::numeric_limits<size_t>::max();
        if (m_nextFreeSlot != NoFreeSlot)
        {
            // If there's a free slot, then use that space and update the linked list of free slots.
            slotToReturn = m_nextFreeSlot;
            m_nextFreeSlot = reinterpret_cast<size_t&>(m_data.at(m_nextFreeSlot));
            new (&m_data.at(slotToReturn)) T();
        }
        else
        {
            // If there's no free slot, append on the end.
            slotToReturn = m_data.size();
            m_data.emplace_back();
        }
        return slotToReturn;
    }
    
    template<typename T>
    inline void SparseVector<T>::Release(size_t index)
    {
        if (index < m_data.size())
        {
            // Explicitly destruct the released element and update the linked list of free slots.
            m_data.at(index).~T();
            reinterpret_cast<size_t&>(m_data.at(index)) = m_nextFreeSlot;
            m_nextFreeSlot = index;
        }
    }

    template<typename T>
    void SparseVector<T>::Clear()
    {
        if constexpr (!AZStd::is_trivially_destructible<T>())
        {
            // Because the memory in the underlying vector is used to store a linked list of the removed items,
            // a destructor could be called on bogus memory when the vector is cleared or destroyed. To fix this,
            // iterate through each free slot and default-construct an object there so it can be safely deleted.
            while (m_nextFreeSlot != NoFreeSlot)
            {
                size_t thisSlot = m_nextFreeSlot;
                m_nextFreeSlot = reinterpret_cast<size_t&>(m_data.at(m_nextFreeSlot));
                new (&m_data.at(thisSlot)) T();
            }
        }
        m_data.clear();
        m_nextFreeSlot = NoFreeSlot;
    }

    template<typename T>
    size_t SparseVector<T>::GetSize() const
    {
        return m_data.size();
    }

    template<typename T>
    T& SparseVector<T>::GetElement(size_t index)
    {
        return m_data.at(index);
    }
    
    template<typename T>
    const T* SparseVector<T>::GetRawData() const
    {
        return m_data.data();
    }
}
