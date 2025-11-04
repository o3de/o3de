/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/RayTracing/RayTracingIndexList.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ
{
    namespace Render
    {
        //! Manages a resource list used by RayTracing.
        //!
        //! Resources are stored in a flat array.  There is also a map that stores the index of the resource
        //! in the array, its reference count, and the index in the indirection list.  This map is used to determine
        //! if the resource is already known, and how to locate its entries in the resource and indirection lists.
        //! 
        //! The indirection list provides a stable index for resources, and is returned to clients of this class.
        //! This allows resources to be moved in the resource array without affecting externally held indices,
        //! since these refer to the indirection list, which in turn points to the resource list.
        template<class TResource>
        class RayTracingResourceList
        {
        public:

            using ResourceVector = AZStd::vector<const TResource*>;
            using IndexVector = AZStd::vector<uint32_t>;

            RayTracingResourceList() = default;
            ~RayTracingResourceList() = default;

            // adds a resource to the list, or increments the reference count, and returns the index of the resource
            // Note: the index returned is an indirection index, meaning it is stable when other entries are removed
            uint32_t AddResource(const TResource* resource);

            // removes a resource from the list, or decrements the reference count
            // Note: removing a resource will not affect any previously returned indices for other resources
            void RemoveResource(const TResource* resource);

            // returns the resource list
            ResourceVector& GetResourceList() { return m_resources; }

            // returns the indirection list
            const IndexVector& GetIndirectionList() const { return m_indirectionList.GetIndexList(); }

            // clears the resource list and all associated state
            void Reset();

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

            using ResourceMap = AZStd::map<const TResource*, IndexMapEntry>;

            ResourceVector m_resources;
            ResourceMap m_resourceMap;
            RayTracingIndexList<1> m_indirectionList;
        };

        template<class TResource>
        uint32_t RayTracingResourceList<TResource>::AddResource(const TResource* resource)
        {
            if (resource == nullptr)
            {
                return InvalidIndex;
            }

            uint32_t indirectionIndex = 0;
            typename ResourceMap::iterator it = m_resourceMap.find(resource);
            if (it == m_resourceMap.end())
            {
                // resource not found, add it to the resource list and the indirection list
                m_resources.push_back(resource);
                uint32_t resourceIndex = aznumeric_cast<uint32_t>(m_resources.size()) - 1;

                indirectionIndex = m_indirectionList.AddEntry(resourceIndex);

                // add the resource map entry containing the true index, indirection index, and reference count
                IndexMapEntry entry;
                entry.m_index = resourceIndex;
                entry.m_indirectionIndex = indirectionIndex;
                entry.m_count = 1;
                m_resourceMap.insert({ resource, entry });
            }
            else
            {
                // resource is already known, update the reference count and return the indirection index
                it->second.m_count++;
                indirectionIndex = it->second.m_indirectionIndex;
            }

            return indirectionIndex;
        }

        template<class TResource>
        void RayTracingResourceList<TResource>::RemoveResource(const TResource* resource)
        {
            if (resource == nullptr)
            {
                return;
            }

            typename ResourceMap::iterator it = m_resourceMap.find(resource);
            AZ_Assert(it != m_resourceMap.end(), "Unable to find resource in the ResourceMap");

            // decrement reference count
            it->second.m_count--;

            // if the reference count is zero then remove the entry from both the map and the main resource list
            if (it->second.m_count == 0)
            {
                uint32_t resourceIndex = it->second.m_index;

                if (resourceIndex < m_resources.size() - 1)
                {
                    // the resource we're removing is in the middle of the list - swap the last entry to this position
                    typename ResourceMap::iterator itLast = m_resourceMap.find(m_resources.back());
                    AZ_Assert(itLast != m_resourceMap.end(), "Unable to find the last resource in the ResourceMap");
                    m_resources[resourceIndex] = m_resources.back();

                    // update the swapped entry with its new index in the resource list
                    itLast->second.m_index = resourceIndex;

                    // update the indirection vector entry of the swapped entry to point to the new position
                    // Note: any indirection indices returned by AddResource for other resources remain stable, this just updates the indirection entry
                    m_indirectionList.SetEntry(itLast->second.m_indirectionIndex, resourceIndex);                 
                }

                // cache the indirection index so that its okay to erase the iterator
                uint32_t cachedIndirectionIndex = it->second.m_indirectionIndex;

                // remove the last entry from the resource list
                m_resources.pop_back();

                // remove the entry from the resource map
                m_resourceMap.erase(it);

                // remove the entry from the indirection list
                m_indirectionList.RemoveEntry(cachedIndirectionIndex);
            }
        }

        template<class TResource>
        void RayTracingResourceList<TResource>::Reset()
        {
            m_resources.clear();
            m_resourceMap.clear();
            m_indirectionList.Reset();
        }
    }
}
