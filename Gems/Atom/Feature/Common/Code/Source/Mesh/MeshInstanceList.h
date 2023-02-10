/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Utils/StableDynamicArray.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/View.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Casting/numeric_cast.h>
#include <RayTracing/RayTracingIndexList.h>
#include <Mesh/MeshInstanceKey.h>

namespace AZ::Render
{
    struct MeshInstanceData
    {
        // Only allow passing in a specific allocator wrapper
        MeshInstanceData() = delete;
        MeshInstanceData(AZStdIAllocator allocator)
            : m_perViewDrawPackets(allocator)
        {
        }

        // The original draw packet, shared by every instance
        RPI::MeshDrawPacket m_drawPacket;

        // We modify the original draw packet each frame with a new instance count and a new root constant
        AZStd::vector<RHI::Ptr<RHI::DrawPacket>, AZStdIAllocator> m_perViewDrawPackets;
        AZStd::vector<AZStd::vector<uint32_t>> m_perViewInstanceData;

        // We store the shaderIntputConstantIndex for m_instanceData here, so we don't have to look it up every frame
        AZStd::vector<RHI::ShaderInputConstantIndex> m_drawSrgInstanceDataIndices;

        // We are assuming that all draw items in a draw packet share the same root constant layout
        RHI::Interval m_drawRootConstantInterval;

        // We store a key to make it faster to remove the instance from the map
        // via the index
        MeshInstanceKey m_key;
    };

    // When adding a new entry, we get back both the index and whether or not
    // a pre-existing entry for that key was found
    struct InsertResult
    {
        StableDynamicArrayWeakHandle<MeshInstanceData> m_handle;
        bool m_wasInserted = false;
    };

    //! Manages an index list used by MeshInstancing.
    //!
    //! This class behaves simlarly to RayTracingResourceList, however the value it uses for the key to the map
    //! is of a different type than the class that is actually stored in the data vector.
    //!
    //! Data is stored in pages.  There is also a map that stores the index of the data
    //! in the array, its reference count, and the index in the indirection list.  This map is used to determine
    //! if the resource is already known, and how to locate its entries in the data vector.
    class MeshInstanceList
    {
    public:
        using WeakHandle = StableDynamicArrayWeakHandle<MeshInstanceData>;
        using OwningHandle = StableDynamicArrayHandle<MeshInstanceData>;
        // adds a data entry to the list, or increments the reference count, and returns the index of the data
        // Note: the index returned is an indirection index, meaning it is stable when other entries are removed
        InsertResult Add(const MeshInstanceKey& key, AZStdIAllocator allocator);

        // removes a data entry from the list, or decrements the reference count
        // Note: removing a data entry will not affect any previously returned indices for other resources
        void Remove(const MeshInstanceKey& key);

        // clears the data list and all associated state
        void Reset();

        uint32_t GetItemCount() const
        {
            return static_cast<uint32_t>(m_pagedDataVector.size());
        }

        MeshInstanceData& operator[](WeakHandle handle);
        const MeshInstanceData& operator[](WeakHandle handle) const;

        struct IndexMapEntry
        {
            IndexMapEntry() = default;

            IndexMapEntry(IndexMapEntry&& rhs)
            {
                m_handle = AZStd::move(rhs.m_handle);
                m_count = rhs.m_count;
            }

            // index of the entry in the main list
            OwningHandle m_handle;

            // reference count
            uint32_t m_count = 0;
        };

        using DataMap = AZStd::unordered_map<MeshInstanceKey, IndexMapEntry>;

    private:
        StableDynamicArray<MeshInstanceData> m_pagedDataVector;
        DataMap m_dataMap;
    };
} // namespace AZ::Render
