/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/Mesh/MeshInstanceManager.h>
#include <AzCore/std/parallel/scoped_lock.h>
namespace AZ
{
    namespace Render
    {
        InsertResult MeshInstanceManager::AddInstance(MeshInstanceKey meshInstanceKey)
        {
            AZStd::scoped_lock lock(m_instanceDataMutex);
            InsertResult result = m_instanceData.Add(meshInstanceKey);
            if (result.m_wasInserted)
            {
                // The MeshInstanceManager is including the key as part of the data vector,
                // so that RemoveInstance can be called via index instead of key. This allows
                // the higher level ModelDataInstance to only have to track the index,
                // not the key, while enabling the underlying SlotMap structure to remove
                // by key, without needing to iterate over the entire DataMap
                m_instanceData[result.m_index].m_key = meshInstanceKey;
            }
            return result;
        }

        void MeshInstanceManager::RemoveInstance(MeshInstanceKey meshInstanceKey)
        {
            AZStd::scoped_lock lock(m_instanceDataMutex);
            m_instanceData.Remove(meshInstanceKey);
        }

        void MeshInstanceManager::RemoveInstance(MeshInstanceIndex meshInstanceIndex)
        {
            AZStd::scoped_lock lock(m_instanceDataMutex);
            m_instanceData.Remove(m_instanceData[meshInstanceIndex].m_key);
        }
    } // namespace Render
} // namespace AZ
