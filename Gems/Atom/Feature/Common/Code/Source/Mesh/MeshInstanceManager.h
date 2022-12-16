/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/parallel/mutex.h>
#include <Mesh/MeshInstanceList.h>

namespace AZ
{
    namespace Render
    {
        //! Represents all the data needed to know if a mesh can be instanced 
        struct MeshInstanceKey
        {
            Data::InstanceId m_modelId = Data::InstanceId::CreateFromAssetId(Data::AssetId(Uuid::CreateNull(), 0));
            uint32_t m_lodIndex = 0;
            uint32_t m_meshIndex = 0;
            Data::InstanceId m_materialId = Data::InstanceId::CreateFromAssetId(Data::AssetId(Uuid::CreateNull(), 0));
            // If anything needs to force instancing off (e.g., if the shader it uses doesn't support instancing),
            // it can set a random uuid here to force it to get a unique key
            Uuid m_forceInstancingOff = Uuid::CreateNull();
            uint8_t m_stencilRef = 0;
            RHI::DrawItemSortKey m_sortKey = 0;

            bool operator<(const MeshInstanceKey& rhs) const
            {
                return AZStd::tie(m_modelId, m_lodIndex, m_meshIndex, m_materialId, m_forceInstancingOff, m_stencilRef, m_sortKey) <
                    AZStd::tie(
                           rhs.m_modelId,
                           rhs.m_lodIndex,
                           rhs.m_meshIndex,
                           rhs.m_materialId,
                           rhs.m_forceInstancingOff,
                           rhs.m_stencilRef,
                           rhs.m_sortKey);
            }

            bool operator==(const MeshInstanceKey& rhs) const
            {
                return m_modelId == rhs.m_modelId && m_lodIndex == rhs.m_lodIndex && m_meshIndex == rhs.m_meshIndex &&
                    m_materialId == rhs.m_materialId && m_forceInstancingOff == rhs.m_forceInstancingOff &&
                    m_stencilRef == rhs.m_stencilRef && m_sortKey == rhs.m_sortKey;

            }

            bool operator!=(const MeshInstanceKey& rhs) const
            {
                return m_modelId != rhs.m_modelId || m_lodIndex != rhs.m_lodIndex || m_meshIndex != rhs.m_meshIndex ||
                    m_materialId != rhs.m_materialId || m_forceInstancingOff != rhs.m_forceInstancingOff ||
                    m_stencilRef != rhs.m_stencilRef || m_sortKey != rhs.m_sortKey;
            }
        };

        struct MeshInstanceData
        {
            RPI::MeshDrawPacket m_drawPacket;

            // We store a key to make it faster to remove the instance from the map
            // via the index
            MeshInstanceKey m_key;
        };

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

            SlotMap<MeshInstanceKey, MeshInstanceData> m_instanceData;

            // Adding and removing entries in a SlotMap is not threadsafe, and should be guarded with a mutex
            AZStd::mutex m_instanceDataMutex;

            AZStd::vector<uint32_t> m_createDrawPacketQueue;
        };
    }
} // namespace AZ

