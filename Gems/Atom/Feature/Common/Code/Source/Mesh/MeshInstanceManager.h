/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/mutex.h>
#include <Mesh/MeshInstanceKey.h>
#include <Mesh/MeshInstanceList.h>

namespace AZ::Render
{
    using MeshInstanceIndex = uint32_t;

    //! The MeshInstanceManager tracks the mesh/material combinations that can be instanced
    class MeshInstanceManager
    {
    public:

        InsertResult AddInstance(MeshInstanceKey meshInstanceData);
        void RemoveInstance(MeshInstanceKey meshInstanceData);
        void RemoveInstance(MeshInstanceIndex index);
        uint32_t GetItemCount() const
        {
            return m_instanceData.GetItemCount();
        }

        MeshInstanceData& operator[](uint32_t index)
        {
            return m_instanceData[index];
        }

    private:

        MeshInstanceList m_instanceData;

        // Adding and removing entries in a MeshInstanceList is not threadsafe, and should be guarded with a mutex
        AZStd::mutex m_instanceDataMutex;

        AZStd::vector<uint32_t> m_createDrawPacketQueue;
    };
} // namespace AZ::Render
