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
        
        const BufferAssetView* ModelLodAsset::GetSemanticBufferAssetView(const AZ::Name& semantic, uint32_t meshIndex) const
        {
            AZ_Assert(meshIndex < m_meshes.size(), "Mesh index out of range");
            return m_meshes[meshIndex].GetSemanticBufferAssetView(semantic);
        }

        void ModelLodAsset::LoadBufferAssets()
        {
            m_indexBuffer.QueueLoad();

            for (auto& streamBuffer : m_streamBuffers)
            {
                streamBuffer.QueueLoad();
            }

            m_indexBuffer.BlockUntilLoadComplete();
            for (auto& streamBuffer : m_streamBuffers)
            {
                streamBuffer.BlockUntilLoadComplete();
            }

            // update buffer asset references in meshes
            for (auto& mesh : m_meshes)
            {
                mesh.LoadBufferAssets();
            }
        }

        void ModelLodAsset::ReleaseBufferAssets()
        {
            m_indexBuffer.Release();

            for (auto& streamBuffer : m_streamBuffers)
            {
                streamBuffer.Release();
            }
             
            for (auto& mesh : m_meshes)
            {
                mesh.ReleaseBufferAssets();
            }
        }

        void ModelLodAsset::Mesh::LoadBufferAssets()
        {
            m_indexBufferAssetView.LoadBufferAsset();

            for (auto& bufferInfo : m_streamBufferInfo)
            {
                bufferInfo.m_bufferAssetView.LoadBufferAsset();
            }
        }
        
        void ModelLodAsset::Mesh::ReleaseBufferAssets()
        {
            m_indexBufferAssetView.ReleaseBufferAsset();

            for (auto& bufferInfo : m_streamBufferInfo)
            {
                bufferInfo.m_bufferAssetView.ReleaseBufferAsset();
            }
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
