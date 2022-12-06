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
        uint32_t MeshInstanceManager::AddInstance(MeshInstanceKey meshInstanceKey)
        {
            AZStd::scoped_lock lock(m_instanceDataMutex);
            return m_instanceData.Add(meshInstanceKey);
        }

        void MeshInstanceManager::RemoveInstance(MeshInstanceKey meshInstanceKey)
        {
            AZStd::scoped_lock lock(m_instanceDataMutex);
            m_instanceData.Remove(meshInstanceKey);
        }
    } // namespace Render
} // namespace AZ
