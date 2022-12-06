/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/Casting/numeric_cast.h>
#include <RayTracing/RayTracingIndexList.h>

namespace AZ
{
    namespace Render
    {
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
        template<class Key, class Data>
        class SlotMap
        {
        public:

            using DataVector = AZStd::vector<Data>;
            using IndexVector = AZStd::vector<uint32_t>;

            SlotMap() = default;
            ~SlotMap() = default;

            // adds a data entry to the list, or increments the reference count, and returns the index of the data
            // Note: the index returned is an indirection index, meaning it is stable when other entries are removed
            uint32_t Add(Key key);

            // removes a data entry from the list, or decrements the reference count
            // Note: removing a data entry will not affect any previously returned indices for other resources
            void Remove(Key key);

            // returns the data list
            DataVector& GetDataList() { return m_data; }

            // returns the indirection list
            const IndexVector& GetIndirectionList() const { return m_indirectionList.GetIndexList(); }

            // returns the list of indices to entries that have not had their data initialized yet
            AZStd::vector<uint32_t>& GetNewEntryList() { return m_newEntryList; }

            // clears the data list and all associated state
            void Reset();

            Data& operator[](uint32_t index);
            const Data& operator[](uint32_t index) const;
        private:

            struct IndexMapEntry
            {
                // index of the entry in the main list
                uint32_t m_index = InvalidIndex;

                // index of the entry in the indirection list
                uint32_t m_indirectionIndex = InvalidIndex;

                // reference count
                uint32_t m_count = 0;
            };

            using DataMap = AZStd::map<Key, IndexMapEntry>;

            DataVector m_data;
            DataMap m_dataMap;
            RayTracingIndexList<1> m_indirectionList;
            AZStd::vector<uint32_t> m_newEntryList;
        };
    }
}
