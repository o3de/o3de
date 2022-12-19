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
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Casting/numeric_cast.h>
#include <RayTracing/RayTracingIndexList.h>

namespace AZ
{
    namespace Render
    {
        static const uint16_t Invalid16BitIndex = AZStd::numeric_limits<uint16_t>::max();
        // When adding a new entry, we get back both the index and whether or not
        // a pre-existing entry for that key was found
        struct InsertResult
        {
            uint32_t m_index = InvalidIndex;
            bool m_wasInserted = false;
        };

        
        struct PageElementIndex
        {
            uint16_t m_pageIndex = Invalid16BitIndex;
            uint16_t m_elementIndex = Invalid16BitIndex;
        };
        //! Manages an index list used by MeshInstancing.
        //!
        //! This class behaves simlarly to RayTracingResourceList, however the value it uses for the key to the map
        //! is of a different type than the class that is actually stored in the data vector.
        //!
        //! Data is stored in a flat array.  There is also a map that stores the index of the data
        //! in the array, its reference count, and the index in the indirection list.  This map is used to determine
        //! if the resource is already known, and how to locate its entries in the resource and indirection lists.
        //! 
        //! The indirection list provides a stable index for resources, and is returned to clients of this class.
        //! This allows resources to be moved in the resource array without affecting externally held indices,
        //! since these refer to the indirection list, which in turn points to the resource list.
        template<class Key, class DataType, uint16_t ElementsPerPage = 512>
        class SlotMap
        {
        public:

            SlotMap() = default;
            ~SlotMap() = default;            

            // adds a data entry to the list, or increments the reference count, and returns the index of the data
            // Note: the index returned is an indirection index, meaning it is stable when other entries are removed
            InsertResult Add(Key key);

            // removes a data entry from the list, or decrements the reference count
            // Note: removing a data entry will not affect any previously returned indices for other resources
            void Remove(Key key);

            // clears the data list and all associated state
            void Reset();

            uint32_t GetItemCount() const { return m_itemCount; }

            DataType& operator[](uint32_t index);
            const DataType& operator[](uint32_t index) const;
        private:

            uint32_t EncodeIndex(uint16_t pageIndex, uint16_t elementIndex) const;


            PageElementIndex DecodeIndex(uint32_t encodedIndex) const;


            struct IndexMapEntry
            {
                // index of the entry in the main list
                uint32_t m_index = InvalidIndex;

                // reference count
                uint32_t m_count = 0;
            };

            using DataMap = AZStd::unordered_map<Key, IndexMapEntry>;
            using DataPage = AZStd::array<DataType, ElementsPerPage>;
            using DataVector = AZStd::vector<DataPage*>;
            using PageFreeList = AZStd::fixed_vector<uint16_t, ElementsPerPage>;
            using IndexFreeListVector = AZStd::vector<PageFreeList>;

            constexpr PageFreeList CreateFreeList() const;

            DataVector m_data;
            IndexFreeListVector m_indexFreeLists;
            DataMap m_dataMap;
            uint32_t m_itemCount = 0;
        };
    }
}
