/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Mesh/MeshInstanceGroupKey.h>

#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/Utils/StableDynamicArray.h>
#include <AtomCore/std/parallel/concurrency_checker.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ::Render
{
    class ModelDataInstance;

    //! This struct contains all the data for a group of meshes that are capable of being rendered
    //! with a single instanced draw call
    struct MeshInstanceGroupData
    {
        // The original draw packet, shared by every instance
        RPI::MeshDrawPacket m_drawPacket;

        // We modify the original draw packet each frame with a new instance count and a new root constant offset
        // The instance count and offset varies per view, so we keep one modifiable copy of the draw packet for each view
        AZStd::vector<RHI::Ptr<RHI::DrawPacket>> m_perViewDrawPackets;

        // All draw items in a draw packet share the same root constant layout
        uint32_t m_drawRootConstantOffset = 0;

        // The current instance group count
        uint32_t m_count = 0;

        // The page that this instance group belongs to
        uint32_t m_pageIndex = 0;

        // We store a key with the data to make it faster to remove the instance without needing to recreate the key
        // or store it with the data for each individual instance
        MeshInstanceGroupKey m_key;

        // A list of ModelDataInstances which are referencing this instance group
        AZStd::set<ModelDataInstance*> m_associatedInstances;

        // Mutex for adding/removing associated ModelDataInstance
        AZStd::mutex m_eventLock;

        // Enable draw motion or not. Set to true if any of mesh instance use this group has the same flag set in their ModelDataInstance
        bool m_isDrawMotion = false;

        // If the group is transparent, sort depth in reverse
        bool m_isTransparent = false;

        // For per-mesh shader options
        // If all the ModelDataInstances within this group are using the same shader option value, then we can apply the mesh shader options to the draw packet.
        // These are two variables which are used to indicate which shader option should be applied and which value is applied.
        // combined shader options from any ModelDataInstance which use this group
        uint32_t m_shaderOptionFlags = 0;

        // flag shader option flags which are in use
        uint32_t m_shaderOptionFlagMask = 0;

        // Update mesh draw packet
        // Return true if DrawPacket was rebuilt
        bool UpdateDrawPacket(const RPI::Scene& parentScene, bool forceUpdate);

        // Update shader option flags for the instance group
        // It goes through the cullable's m_shaderOptionFlags of each associated ModelDataInstance and get combined m_shaderOptionFlags and m_shaderOptionFlagMask
        // Return true if flags or mask changed.
        bool UpdateShaderOptionFlags();

        // Add/remove an associated ModelDataInstance (thread safe)
        void AddAssociatedInstance(ModelDataInstance* instance);
        void RemoveAssociatedInstance(ModelDataInstance* instance);
    };

    //! Manages all the instance groups used by mesh instancing.
    //! 
    //! Data is stored in pages. There is also a map that stores a handle the data in the array, and its reference count.
    //! This map is used to determine if the instance group is already known, and how to access it.
    class MeshInstanceGroupList
    {
    public:
        using WeakHandle = StableDynamicArrayWeakHandle<MeshInstanceGroupData>;
        using OwningHandle = StableDynamicArrayHandle<MeshInstanceGroupData>;
        using StableDynamicArrayType = StableDynamicArray<MeshInstanceGroupData, 4096>;
        using ParallelRanges = StableDynamicArrayType::ParallelRanges;
        // When adding a new entry, we get back both the index and the count of meshes in the group after inserting
        // The count can be used to determine if this is the first mesh in the group (and thus intialization may be required)
        // As well as to determine if the mesh has reached the threshold at which it can become instanced,
        // if support for such a threshold is added
        struct InsertResult
        {
            StableDynamicArrayWeakHandle<MeshInstanceGroupData> m_handle;
            uint32_t m_instanceCount = 0;
            uint32_t m_pageIndex = 0;
        };

        // Adds a new instance group if none with a matching key exists, or increments the reference count if one already does,
        // and returns the handle to data and the number of instances in the group
        InsertResult Add(const MeshInstanceGroupKey& key);

        // Decrements the reference count of an instance group, and removes the data if the count drops to 0
        // Removing a instance group will not affect any previously returned handles for other instance groups
        void Remove(const MeshInstanceGroupKey& key);

        // Returns the number of instance groups
        uint32_t GetInstanceGroupCount() const;

        // Returns parallel ranges for the underlying instance group data. Each range corresponds to a page of data.
        ParallelRanges GetParallelRanges();

        // Direct access via handle is thread safe while adding and removing other instance groups
        MeshInstanceGroupData& operator[](WeakHandle handle);
        const MeshInstanceGroupData& operator[](WeakHandle handle) const;

        struct IndexMapEntry
        {
            IndexMapEntry() = default;

            IndexMapEntry(IndexMapEntry&& rhs)
            {
                m_handle = AZStd::move(rhs.m_handle);
                m_count = rhs.m_count;
            }

            // Handle to the entry in the stable array
            OwningHandle m_handle;

            // Reference count
            uint32_t m_count = 0;
        };

        using DataMap = AZStd::unordered_map<MeshInstanceGroupKey, IndexMapEntry>;

    private:
        StableDynamicArrayType m_instanceGroupData;
        DataMap m_dataMap;
        AZStd::concurrency_checker m_instanceDataConcurrencyChecker;
    };
} // namespace AZ::Render
