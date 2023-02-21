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
        using Handle = MeshInstanceList::WeakHandle;

        InsertResult AddInstance(MeshInstanceKey meshInstanceData);
        void RemoveInstance(MeshInstanceKey meshInstanceData);
        void RemoveInstance(Handle handle);
        void SetAllocator(IAllocator* allocator)
        {
            m_instanceDataAllocator = allocator;
        }
        uint32_t GetItemCount() const
        {
            return m_instanceData.GetItemCount();
        }

        MeshInstanceData& operator[](Handle handle)
        {
            return m_instanceData[handle];
        }

        AZStd::vector<AZStd::pair<StableDynamicArray<MeshInstanceData>::pageIterator, StableDynamicArray<MeshInstanceData>::pageIterator>>
        GetParallelRanges()
        {
            return m_instanceData.GetParallelRanges();
        }

    private:

        MeshInstanceList m_instanceData;
        IAllocator* m_instanceDataAllocator;
        // Adding and removing entries in a MeshInstanceList is not threadsafe, and should be guarded with a mutex
        AZStd::mutex m_instanceDataMutex;

        AZStd::vector<uint32_t> m_createDrawPacketQueue;
    };
} // namespace AZ::Render
