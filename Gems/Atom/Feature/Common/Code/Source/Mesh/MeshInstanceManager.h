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
            Data::AssetId m_modelAssetId;
            uint32_t m_lodIndex;
            uint32_t m_meshIndex;
            Data::AssetId m_materialAssetId;
            // If anything needs to force instancing off (e.g., if the shader it uses doesn't support instancing),
            // it can set a random uuid here to force it to get a unique key
            Uuid m_forceInstancingOff;

            bool operator<(const MeshInstanceKey& rhs) const
            {
                return m_modelAssetId < rhs.m_modelAssetId || m_lodIndex < rhs.m_lodIndex || m_meshIndex < rhs.m_meshIndex ||
                    m_materialAssetId < rhs.m_materialAssetId;
            }

            
            bool operator==(const MeshInstanceKey& rhs) const
            {
                return m_modelAssetId == rhs.m_modelAssetId && m_lodIndex == rhs.m_lodIndex && m_meshIndex == rhs.m_meshIndex &&
                    m_materialAssetId == rhs.m_materialAssetId;

            }
            bool operator!=(const MeshInstanceKey& rhs) const
            {
                return m_modelAssetId != rhs.m_modelAssetId || m_lodIndex != rhs.m_lodIndex || m_meshIndex != rhs.m_meshIndex ||
                    m_materialAssetId != rhs.m_materialAssetId;
            }
        };

        struct MeshInstanceData
        {
            RPI::MeshDrawPacket m_drawPacket;
        };

        //! The MeshInstanceManager tracks the mesh/material combinations that can be instanced
        class MeshInstanceManager
        {
        public:
            uint32_t AddInstance(MeshInstanceKey meshInstanceData);
            void RemoveInstance(MeshInstanceKey meshInstanceData);

            AZStd::vector<uint32_t>& GetUninitializedInstances()
            {
                return m_instanceData.GetNewEntryList();
            }

            AZStd::vector<MeshInstanceData>& GetDataList()
            {
                return m_instanceData.GetDataList();
            }

        private:

            SlotMap<MeshInstanceKey, MeshInstanceData> m_instanceData;

            // Adding and removing entries in a SlotMap is not threadsafe, and should be guarded with a mutex
            AZStd::mutex m_instanceDataMutex;

            AZStd::vector<uint32_t> m_createDrawPacketQueue;
        };
    }
}
