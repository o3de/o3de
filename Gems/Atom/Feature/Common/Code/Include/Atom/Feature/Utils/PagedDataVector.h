/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/limits.h>
#include <cstdint>

namespace AZ::Render
{    
    static uint32_t Invalid32BitIndex = AZStd::numeric_limits<uint32_t>::max();
    static uint16_t Invalid16BitIndex = AZStd::numeric_limits<uint16_t>::max();

    struct PageElementIndex
    {
        uint16_t m_pageIndex = Invalid16BitIndex;
        uint16_t m_elementIndex = Invalid16BitIndex;
    };

    template<class Key, class DataType, uint16_t ElementsPerPage = 512>
    class PagedDataVector
    {
    public:
        PagedDataVector() = default;
        ~PagedDataVector() = default;


        // adds a data entry to the list, or increments the reference count, and returns the index of the data
        // Note: the index returned is an indirection index, meaning it is stable when other entries are removed
        uint32_t Add();

        // removes a data entry from the list, or decrements the reference count
        // Note: removing a data entry will not affect any previously returned indices for other resources
        void Remove(uint32_t index);

        // clears the data list and all associated state
        void Reset();

        uint32_t GetItemCount() const
        {
            return m_itemCount;
        }

        DataType& operator[](uint32_t index);
        const DataType& operator[](uint32_t index) const;

    private:

        uint32_t EncodeIndex(uint16_t pageIndex, uint16_t elementIndex) const;

        PageElementIndex DecodeIndex(uint32_t encodedIndex) const;

        using DataPage = AZStd::array<DataType, ElementsPerPage>;
        using DataVector = AZStd::vector<DataPage*>;
        using PageFreeList = AZStd::fixed_vector<uint16_t, ElementsPerPage>;
        using IndexFreeListVector = AZStd::vector<PageFreeList>;

        constexpr PageFreeList CreateFreeList() const;

        DataVector m_data;
        IndexFreeListVector m_indexFreeLists;
        uint32_t m_itemCount = 0;
    };
} // namespace AZ::Render

#include <Atom/Feature/Utils/PagedDataVector.inl>
