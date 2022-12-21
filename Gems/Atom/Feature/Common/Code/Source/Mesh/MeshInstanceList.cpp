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
    InsertResult MeshInstanceList::Add(const MeshInstanceKey& key)
    {
        uint32_t dataIndex = InvalidIndex;
        bool wasIndexInserted = false;

        typename DataMap::iterator it = m_dataMap.find(key);
        if (it == m_dataMap.end())
        {
            dataIndex = m_pagedDataVector.Add();

            // add the data map entry containing the dataIndex and reference count
            IndexMapEntry entry;
            entry.m_index = dataIndex;
            entry.m_count = 1;
            m_dataMap.insert({ key, entry });

            wasIndexInserted = true;
        }
        else
        {
            // data is already known, update the reference count and return the index
            it->second.m_count++;
            dataIndex = it->second.m_index;
        }

        return InsertResult{ dataIndex, wasIndexInserted };
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
            uint32_t dataIndex = it->second.m_index;

            m_pagedDataVector.Remove(dataIndex);

            // Remove it from the data map
            m_dataMap.erase(it);
        }
    }

    MeshInstanceData& MeshInstanceList::operator[](uint32_t index)
    {
        return m_pagedDataVector[index];
    }

    const MeshInstanceData& MeshInstanceList::operator[](uint32_t index) const
    {
        return m_pagedDataVector[index];
    }

    void MeshInstanceList::Reset()
    {
        m_dataMap.clear();
        m_pagedDataVector.Reset();
    }
} // namespace AZ::Render
