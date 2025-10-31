/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Mesh/MeshInstanceManager.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ
{
    namespace Render
    {
        MeshInstanceManager::InsertResult MeshInstanceManager::AddInstance(MeshInstanceGroupKey meshInstanceGroupKey)
        {
            AZStd::scoped_lock lock(m_instanceDataMutex);
            MeshInstanceManager::InsertResult result = m_instanceData.Add(meshInstanceGroupKey);
            if (result.m_instanceCount == 1)
            {
                // The MeshInstanceManager is including the key as part of the data vector,
                // so that RemoveInstance can be called via handle instead of key. This allows
                // the higher level ModelDataInstance to only have to track the handle,
                // not the key, while enabling the underlying MeshInstanceList structure to remove
                // by key, without needing to iterate over the entire DataMap
                m_instanceData[result.m_handle].m_key = meshInstanceGroupKey;
            }
            return result;
        }

        void MeshInstanceManager::RemoveInstance(MeshInstanceGroupKey meshInstanceGroupKey)
        {
            AZStd::scoped_lock lock(m_instanceDataMutex);
            m_instanceData.Remove(meshInstanceGroupKey);
        }

        void MeshInstanceManager::RemoveInstance(Handle handle)
        {
            AZStd::scoped_lock lock(m_instanceDataMutex);
            m_instanceData.Remove(m_instanceData[handle].m_key);
        }
        
        uint32_t MeshInstanceManager::GetInstanceGroupCount() const
        {
            return m_instanceData.GetInstanceGroupCount();
        }

        MeshInstanceGroupData& MeshInstanceManager::operator[](Handle handle)
        {
            return m_instanceData[handle];
        }

        auto MeshInstanceManager::GetParallelRanges() -> ParallelRanges
        {
            return m_instanceData.GetParallelRanges();
        }
    } // namespace Render
} // namespace AZ
