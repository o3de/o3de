/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/PagedDataVector.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
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
        // The original draw packet, shared by every instance
        RPI::MeshDrawPacket m_drawPacket;

        // We modify the original draw packet each frame with a new instance count and a new root constant
        RHI::Ptr<RHI::DrawPacket> m_clonedDrawPacket;

        // We store the shaderIntputConstantIndex for m_instanceData here, so we don't have to look it up every frame
        AZStd::vector<RHI::ShaderInputConstantIndex> m_drawSrgInstanceDataIndices;


        // We store a key to make it faster to remove the instance from the map
        // via the index
        MeshInstanceKey m_key;
    };

    // When adding a new entry, we get back both the index and whether or not
    // a pre-existing entry for that key was found
    struct InsertResult
    {
        uint32_t m_index = AZStd::numeric_limits<uint32_t>::max();
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
        // adds a data entry to the list, or increments the reference count, and returns the index of the data
        // Note: the index returned is an indirection index, meaning it is stable when other entries are removed
        InsertResult Add(const MeshInstanceKey& key);

        // removes a data entry from the list, or decrements the reference count
        // Note: removing a data entry will not affect any previously returned indices for other resources
        void Remove(const MeshInstanceKey& key);

        // clears the data list and all associated state
        void Reset();

        uint32_t GetItemCount() const
        {
            return m_pagedDataVector.GetItemCount();
        }

        MeshInstanceData& operator[](uint32_t index);
        const MeshInstanceData& operator[](uint32_t index) const;

        struct IndexMapEntry
        {
            // index of the entry in the main list
            uint32_t m_index = Invalid32BitIndex;

            // reference count
            uint32_t m_count = 0;
        };

        using DataMap = AZStd::unordered_map<MeshInstanceKey, IndexMapEntry>;

    private:
        PagedDataVector<MeshInstanceKey, MeshInstanceData> m_pagedDataVector;
        DataMap m_dataMap;
    };
} // namespace AZ::Render
