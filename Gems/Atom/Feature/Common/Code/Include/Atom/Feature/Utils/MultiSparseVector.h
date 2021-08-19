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
    //! MultiSparseVector works similarly to SparseVector, but supports multiple underlying vectors
    //! templated to several different types. A separate underlying vector is created for each template
    //! type and elements are reserved and released in all vectors simultaneously. Each type can be
    //! retrieved individually by index with GetElement<ArrayIndex>(index) where the first Type is
    //! ArrayIndex 0, the next Type is ArrayIndex 1 etc.
    //! See SparseVector.h for more details.
    
    template<typename ... Ts>
    class MultiSparseVector
    {
    public:

        MultiSparseVector();
        
        //! Reserves elements in the underlying vectors and returns the index to those elements.
        size_t Reserve();

        //! Releases elements with the given index so they can be reused.
        void Release(size_t index);

        //! Clears all the data from the underlying vectors and resets the size to 0
        void Clear();

        //! Returns the size of the underlying vectors. This is not the same as the number of
        //! valid elements in the vectors since there can be empty slots.
        size_t GetSize() const;
        
        //! Returns a reference to the element at a given index in the ArrayIndex vector.
        template<size_t ArrayIndex>
        auto& GetElement(size_t index);
        
        //! Returns a pointer to the raw data for the ArrayIndex vector.
        template<size_t ArrayIndex>
        auto* GetRawData() const;

    private:

        static constexpr size_t NoFreeSlot = std::numeric_limits<size_t>::max();
        static constexpr size_t InitialReservedCount = 128;
        
        using Fn = void(&)(AZStd::vector<Ts>& ...);

        // Variadic convenience functions

        template <class... Args>
        static void ClearFunction(Args&... args)
        {
            (args.clear(), ...);
        }
        
        template <class... Args>
        static void EmplaceBackFunction(Args&... args)
        {
            (args.emplace_back(), ...);
        }
        
        template <class... Args>
        static void ReserveCapacityFunction(Args&... args)
        {
            (args.reserve(InitialReservedCount), ...);
        }

        template <typename T>
        static void InitializeElement(size_t index, T& container)
        {
            container.at(index) = {};
        }

        template <typename T>
        static void DeleteElement(size_t index, T& container)
        {
            AZStd::destroy_at(&container.at(index));
        }

        size_t m_nextFreeSlot = NoFreeSlot;
        AZStd::tuple<AZStd::vector<Ts>...> m_data;
    };
    
    template<typename ... Ts>
    MultiSparseVector<Ts...>::MultiSparseVector()
    {
        static_assert(sizeof(AZStd::tuple_element_t<0, AZStd::tuple<Ts...>>) >= sizeof(size_t),
            "Data stored in the first element of MultiSparseVector must be at least as large as a size_t.");

        // Reserve some initial capacity in the vectors based on InitialReservedCount.
        AZStd::apply(static_cast<Fn>(ReserveCapacityFunction), m_data);
    }

    template<typename ... Ts>
    inline size_t MultiSparseVector<Ts...>::Reserve()
    {
        size_t slotToReturn = std::numeric_limits<size_t>::max();
        if (m_nextFreeSlot != NoFreeSlot)
        {
            // If there's a free slot, then use that space and update the linked list of free slots.
            slotToReturn = m_nextFreeSlot;
            m_nextFreeSlot = reinterpret_cast<size_t&>(GetElement<0>(m_nextFreeSlot));
            AZStd::apply([&](auto&... args){ (InitializeElement(slotToReturn, args), ...); }, m_data);
        }
        else
        {
            // If there's no free slot, append on the end.
            slotToReturn = GetSize();
            AZStd::apply(static_cast<Fn>(EmplaceBackFunction), m_data);
        }
        return slotToReturn;
    }
    
    template<typename ... Ts>
    inline void MultiSparseVector<Ts...>::Release(size_t index)
    {
        AZ_Assert(index < GetSize(), "MultiSparseVector::Release() index out of bounds.");
        if (index < GetSize())
        {
            // Explicitly destruct the released elements and update the linked list of free slots.
            AZStd::apply([&](auto&... args){ (DeleteElement(index, args), ...); }, m_data);
            reinterpret_cast<size_t&>(GetElement<0>(index)) = m_nextFreeSlot;
            m_nextFreeSlot = index;
        }
    }
    
    template<typename ... Ts>
    void MultiSparseVector<Ts...>::Clear()
    {
        AZStd::apply(static_cast<Fn>(ClearFunction), m_data);
        m_nextFreeSlot = NoFreeSlot;
    }
    
    template<typename ... Ts>
    size_t MultiSparseVector<Ts...>::GetSize() const
    {
        return AZStd::get<0>(m_data).size();
    }
    
    template<typename ... Ts>
    template<size_t ArrayIndex>
    auto& MultiSparseVector<Ts...>::GetElement(size_t index)
    {
        return AZStd::get<ArrayIndex>(m_data).at(index);
    }
    
    template<typename ... Ts>
    template<size_t ArrayIndex>
    auto* MultiSparseVector<Ts...>::GetRawData() const
    {
        return AZStd::get<ArrayIndex>(m_data).data();
    }
}
