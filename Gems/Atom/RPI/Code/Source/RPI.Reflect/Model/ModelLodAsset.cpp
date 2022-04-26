/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>

#include <AzCore/Asset/AssetSerializer.h>

namespace AZ
{
    namespace RPI
    {
        const char* ModelLodAsset::DisplayName = "ModelLodAsset";
        const char* ModelLodAsset::Group = "Model";
        const char* ModelLodAsset::Extension = "azlod";

        void ModelLodAsset::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelLodAsset>()
                    ->Version(0)
                    ->Field("Meshes", &ModelLodAsset::m_meshes)
                    ->Field("Aabb", &ModelLodAsset::m_aabb)
                    ->Field("StreamBuffers", &ModelLodAsset::m_streamBuffers)
                    ->Field("IndexBufferView", &ModelLodAsset::m_indexBuffer)
                    ;
            }

            Mesh::Reflect(context);
        }
        
        void ModelLodAsset::Mesh::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelLodAsset::Mesh>()
                    ->Version(1)
                    ->Field("Name", &ModelLodAsset::Mesh::m_name)
                    ->Field("AABB", &ModelLodAsset::Mesh::m_aabb)
                    ->Field("MaterialSlotId", &ModelLodAsset::Mesh::m_materialSlotId)
                    ->Field("IndexBufferAssetView", &ModelLodAsset::Mesh::m_indexBufferAssetView)
                    ->Field("StreamBufferInfo", &ModelLodAsset::Mesh::m_streamBufferInfo)
                    ;
            }

            StreamBufferInfo::Reflect(context);
        }

        void ModelLodAsset::Mesh::StreamBufferInfo::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelLodAsset::Mesh::StreamBufferInfo>()
                    ->Version(1)
                    ->Field("Semantic", &ModelLodAsset::Mesh::StreamBufferInfo::m_semantic)
                    ->Field("CustomName", &ModelLodAsset::Mesh::StreamBufferInfo::m_customName)
                    ->Field("BufferAssetView", &ModelLodAsset::Mesh::StreamBufferInfo::m_bufferAssetView)
                    ;
            }
        }

        uint32_t ModelLodAsset::Mesh::GetVertexCount() const
        {
            // Index 0 here is not special. All stream buffer views owned by this mesh should
            // view the same number of vertices. It doesn't make sense to be viewing 30 positions
            // but only 20 normals since we're using an index buffer model. 
            return m_streamBufferInfo[0].m_bufferAssetView.GetBufferViewDescriptor().m_elementCount;
        }

        uint32_t ModelLodAsset::Mesh::GetIndexCount() const
        {
            return m_indexBufferAssetView.GetBufferViewDescriptor().m_elementCount;
        }

        ModelMaterialSlot::StableId ModelLodAsset::Mesh::GetMaterialSlotId() const
        {
            return m_materialSlotId;
        }

        const AZ::Name& ModelLodAsset::Mesh::GetName() const
        {
            return m_name;
        }

        const AZ::Aabb& ModelLodAsset::Mesh::GetAabb() const
        {
            return m_aabb;
        }

        const BufferAssetView& ModelLodAsset::Mesh::GetIndexBufferAssetView() const
        {
            return m_indexBufferAssetView;
        }

        AZStd::span<const ModelLodAsset::Mesh::StreamBufferInfo> ModelLodAsset::Mesh::GetStreamBufferInfoList() const
        {
            return AZStd::span<const ModelLodAsset::Mesh::StreamBufferInfo>(m_streamBufferInfo);
        }

        void ModelLodAsset::AddMesh(const Mesh& mesh)
        {
            m_meshes.push_back(mesh);

            Aabb meshAabb = mesh.GetAabb();

            m_aabb.AddAabb(meshAabb);
        }

        AZStd::span<const ModelLodAsset::Mesh> ModelLodAsset::GetMeshes() const
        {
            return AZStd::span<const ModelLodAsset::Mesh>(m_meshes);
        }

        const AZ::Aabb& ModelLodAsset::GetAabb() const
        {
            return m_aabb;
        }
        
        const BufferAssetView* ModelLodAsset::Mesh::GetSemanticBufferAssetView(const AZ::Name& semantic) const
        {
            const AZStd::span<const ModelLodAsset::Mesh::StreamBufferInfo>& streamBufferList = GetStreamBufferInfoList();

            for (const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : streamBufferList)
            {
                if (streamBufferInfo.m_semantic.m_name == semantic)
                {
                    return &streamBufferInfo.m_bufferAssetView;
                }
            }

            return nullptr;
        }

        void ModelLodAsset::SetReady()
        {
            m_status = Data::AssetData::AssetStatus::Ready;
        }
    } // namespace RPI
} // namespace AZ
