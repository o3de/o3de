/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/MeshInstanceList.h>
#include <AzCore/std/numeric.h>

namespace AZ::Render
{
    InsertResult MeshInstanceList::Add(const MeshInstanceKey& key, AZ::AZStdIAllocator allocator)
    {
        WeakHandle handle;
        bool wasIndexInserted = false;

        typename DataMap::iterator it = m_dataMap.find(key);
        if (it == m_dataMap.end())
        {
            // add the data map entry containing the dataIndex and reference count
            IndexMapEntry entry;
            entry.m_handle = m_pagedDataVector.emplace(MeshInstanceData{ allocator });
            entry.m_count = 1;
            handle = entry.m_handle.GetWeakHandle();

            m_dataMap.emplace(AZStd::make_pair(key, AZStd::move(entry)));

            wasIndexInserted = true;
        }
        else
        {
            // data is already known, update the reference count and return the index
            it->second.m_count++;
            handle = it->second.m_handle.GetWeakHandle();
        }

        return InsertResult{ handle, wasIndexInserted };
    }

    void MeshInstanceList::Remove(const MeshInstanceKey& key)
    {
        typename DataMap::iterator it = m_dataMap.find(key);
        if (it == m_dataMap.end())
        {
            AZ_Assert(false, "Unable to find key in the DataMap");
            return;
        }

        // decrement reference count
        it->second.m_count--;

        // if the reference count is zero then remove the entry from both the map and the main data list
        if (it->second.m_count == 0)
        {
            // Remove it from the data map
            // The owning handle will go out of scope, and it will be erased from the underlying array as well
            m_dataMap.erase(it);
        }
    }

    MeshInstanceData& MeshInstanceList::operator[](WeakHandle handle)
    {
        return m_pagedDataVector.GetData(handle);
    }

    const MeshInstanceData& MeshInstanceList::operator[](WeakHandle handle) const
    {
        return m_pagedDataVector.GetData(handle);
    }

    void MeshInstanceList::Reset()
    {
        m_dataMap.clear();
        m_pagedDataVector.ReleaseEmptyPages();
    }
} // namespace AZ::Render
