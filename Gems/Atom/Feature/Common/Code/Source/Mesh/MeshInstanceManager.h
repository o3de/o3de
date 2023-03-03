/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/mutex.h>
#include <Mesh/MeshInstanceGroupKey.h>
#include <Mesh/MeshInstanceGroupList.h>

namespace AZ::Render
{
    //! The MeshInstanceManager tracks the mesh/material combinations that can be instanced
    class MeshInstanceManager
    {
    public:
        using Handle = MeshInstanceGroupList::WeakHandle;
        using InsertResult = MeshInstanceGroupList::InsertResult;

        MeshInstanceManager::InsertResult AddInstance(MeshInstanceGroupKey meshInstanceGroupData);

        void RemoveInstance(MeshInstanceGroupKey meshInstanceGroupData);

        void RemoveInstance(Handle handle);

        uint32_t GetInstanceGroupCount() const;

        MeshInstanceGroupData& operator[](Handle handle);

        AZStd::vector<
            AZStd::pair<StableDynamicArray<MeshInstanceGroupData>::pageIterator, StableDynamicArray<MeshInstanceGroupData>::pageIterator>>
        GetParallelRanges();

    private:

        MeshInstanceGroupList m_instanceData;

        // Adding and removing entries in a MeshInstanceList is not threadsafe, and should be guarded with a mutex
        AZStd::mutex m_instanceDataMutex;

        AZStd::vector<uint32_t> m_createDrawPacketQueue;
    };
} // namespace AZ::Render
