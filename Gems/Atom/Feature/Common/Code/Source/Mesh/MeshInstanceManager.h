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
        using ParallelRanges = MeshInstanceGroupList::ParallelRanges;

        //! Increase the ref-count for an instance group if one already exists for the given key, or add a new instance group if it doesn't exist.
        //! Returns an InsertResult with a weak handle to the data and the ref-count for the instance group after adding this instance.
        InsertResult AddInstance(MeshInstanceGroupKey meshInstanceGroupData);

        //! Decrease the ref-count for an instance group, and remove it if the ref-count drops to 0.
        //! The MeshInstanceManager keeps a copy of the MeshInstanceGroupKey alongside the data, so
        //! removing by handle is just as performance as removing by key.
        void RemoveInstance(MeshInstanceGroupKey meshInstanceGroupData);

        //! Decrease the ref-count for an instance group, and remove it if the ref-count drops to 0.
        //! The MeshInstanceManager keeps a copy of the MeshInstanceGroupKey alongside the data, so
        //! removing by handle is just as performance as removing by key.
        void RemoveInstance(Handle handle);

        //! Get the total number of instance groups being managed by the MeshInstanceManager
        uint32_t GetInstanceGroupCount() const;

        //! Constant O(1) access to a MeshInstanceGroup via its handle
        MeshInstanceGroupData& operator[](Handle handle);

        //! Get begin and end iterators for each page in the MeshInstanceGroup, which can be processed in parallel
        ParallelRanges GetParallelRanges();

    private:

        MeshInstanceGroupList m_instanceData;

        // Adding and removing entries in a MeshInstanceList is not threadsafe, and should be guarded with a mutex
        AZStd::mutex m_instanceDataMutex;

        AZStd::vector<uint32_t> m_createDrawPacketQueue;
    };
} // namespace AZ::Render
