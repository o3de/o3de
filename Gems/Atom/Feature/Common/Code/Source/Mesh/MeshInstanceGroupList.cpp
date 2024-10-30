/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/MeshFeatureProcessor.h>
#include <Mesh/MeshInstanceGroupList.h>
#include <AzCore/std/numeric.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ::Render
{
    bool MeshInstanceGroupData::UpdateDrawPacket(const RPI::Scene& parentScene, bool forceUpdate)
    {
        if (m_drawPacket.Update(parentScene, forceUpdate))
        {
            // Clear any cached draw packets, since they need to be re-created
            m_perViewDrawPackets.clear();
            for (auto modelDataInstance : m_associatedInstances)
            {
                modelDataInstance->HandleDrawPacketUpdate(m_key.m_lodIndex, m_key.m_meshIndex, m_drawPacket);
            }
            return true;
        }
        return false;
    }

    bool MeshInstanceGroupData::UpdateShaderOptionFlags()
    {
        // Shader options are either set or being unspecified (which means use the global value).
        // We only set a shader option if ALL instances have the same value. Otherwise, we leave it unspecified.
        uint32_t newShaderOptionFlagMask = ~0u;      // Defaults to all shaders options being specified. 
                                                    // We only disable one if we find a different between the instances.
        uint32_t newShaderOptionFlags = m_shaderOptionFlags;
        if (auto it = m_associatedInstances.begin(); it != m_associatedInstances.end())
        {
            newShaderOptionFlags = (*it)->GetCullable().m_shaderOptionFlags;
            uint32_t lastShaderOptionFlags = newShaderOptionFlags;
            for (++it; it != m_associatedInstances.end(); ++it)
            {
                ModelDataInstance* modelDataInstance = *it;
                // If the shader option flag of different intances are different, the mask for the flag is 0, which means the flag is
                // unspecified.
                newShaderOptionFlagMask &= ~(lastShaderOptionFlags ^ modelDataInstance->GetCullable().m_shaderOptionFlags);
                // if the option flag has same value, keep the value.
                lastShaderOptionFlags = modelDataInstance->GetCullable().m_shaderOptionFlags;
                newShaderOptionFlags &= lastShaderOptionFlags;                
            }
        }

        // return ture if the shader option flags or mask changed. 
        if (newShaderOptionFlags != m_shaderOptionFlags || newShaderOptionFlagMask != m_shaderOptionFlagMask)
        {
            m_shaderOptionFlags = newShaderOptionFlags;
            m_shaderOptionFlagMask = newShaderOptionFlagMask;
            return true;
        }
        return false;
    }

    void MeshInstanceGroupData::AddAssociatedInstance(ModelDataInstance* instance)
    {
        AZStd::scoped_lock<AZStd::mutex> scopedLock(m_eventLock);
        m_associatedInstances.insert(instance);

    }

    void MeshInstanceGroupData::RemoveAssociatedInstance(ModelDataInstance* instance)
    {
        AZStd::scoped_lock<AZStd::mutex> scopedLock(m_eventLock);
        m_associatedInstances.erase(instance);
    }

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

        // Keep track of the count from the map in the data itself
        it->second.m_handle->m_count = it->second.m_count;

        m_instanceDataConcurrencyChecker.soft_unlock();
        return MeshInstanceGroupList::InsertResult{ it->second.m_handle.GetWeakHandle(), it->second.m_count, static_cast<uint32_t>(m_instanceGroupData.GetPageIndex(it->second.m_handle))};
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
