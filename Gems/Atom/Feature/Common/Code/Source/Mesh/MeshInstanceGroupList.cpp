/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/MeshInstanceGroupList.h>
#include <AzCore/std/numeric.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ::Render
{
    MeshInstanceGroupList::InsertResult MeshInstanceGroupList::Add(const MeshInstanceGroupKey& key)
    {
        // It is not safe to have multiple threads Add and/or Remove at the same time
        m_instanceDataConcurrencyChecker.soft_lock();

        typename DataMap::iterator it = m_dataMap.find(key);
        if (it == m_dataMap.end())
        {
            // Add the data map entry containing the handle and reference count
            IndexMapEntry entry;
            entry.m_handle = m_instanceGroupData.emplace();
            entry.m_count = 1;
            it = m_dataMap.emplace(AZStd::make_pair(key, AZStd::move(entry))).first;
        }
        else
        {
            // Data is already known, update the reference count and return the index
            it->second.m_count++;
        }

        m_instanceDataConcurrencyChecker.soft_unlock();
        return MeshInstanceGroupList::InsertResult{ it->second.m_handle.GetWeakHandle(), it->second.m_count};
    }

    void MeshInstanceGroupList::Remove(const MeshInstanceGroupKey& key)
    {
        // It is not safe to have multiple threads Add and/or Remove at the same time
        m_instanceDataConcurrencyChecker.soft_lock();

        typename DataMap::iterator it = m_dataMap.find(key);
        if (it == m_dataMap.end())
        {
            AZ_Assert(false, "Unable to find key in the DataMap");
            m_instanceDataConcurrencyChecker.soft_unlock();
            return;
        }

        it->second.m_count--;

        if (it->second.m_count == 0)
        {
            // Remove it from the data map
            // The owning handle will go out of scope, which will erase it from the underlying array as well
            m_dataMap.erase(it);
        }

        m_instanceDataConcurrencyChecker.soft_unlock();
    }
    
    uint32_t MeshInstanceGroupList::GetInstanceGroupCount() const
    {
        return static_cast<uint32_t>(m_instanceGroupData.size());
    }
        
    auto MeshInstanceGroupList::GetParallelRanges() -> ParallelRanges
    {
        return m_instanceGroupData.GetParallelRanges();
    }

    MeshInstanceGroupData& MeshInstanceGroupList::operator[](WeakHandle handle)
    {
        return *handle;
    }

    const MeshInstanceGroupData& MeshInstanceGroupList::operator[](WeakHandle handle) const
    {
        return *handle;
    }
} // namespace AZ::Render