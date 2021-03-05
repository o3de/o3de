/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

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
                    ;
            }

            Mesh::Reflect(context);
        }

        void ModelLodAsset::Mesh::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelLodAsset::Mesh>()
                    ->Version(0)
                    ->Field("Material", &ModelLodAsset::Mesh::m_materialAsset)
                    ->Field("Name", &ModelLodAsset::Mesh::m_name)
                    ->Field("AABB", &ModelLodAsset::Mesh::m_aabb)
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

        const Data::Asset <MaterialAsset>& ModelLodAsset::Mesh::GetMaterialAsset() const
        {
            return m_materialAsset;
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

        AZStd::array_view<ModelLodAsset::Mesh::StreamBufferInfo> ModelLodAsset::Mesh::GetStreamBufferInfoList() const
        {
            return AZStd::array_view<ModelLodAsset::Mesh::StreamBufferInfo>(m_streamBufferInfo);
        }

        void ModelLodAsset::AddMesh(const Mesh& mesh)
        {
            m_meshes.push_back(mesh);

            Aabb meshAabb = mesh.GetAabb();

            m_aabb.AddAabb(meshAabb);
        }

        AZStd::array_view<ModelLodAsset::Mesh> ModelLodAsset::GetMeshes() const
        {
            return AZStd::array_view<ModelLodAsset::Mesh>(m_meshes);
        }

        const AZ::Aabb& ModelLodAsset::GetAabb() const
        {
            return m_aabb;
        }

        const BufferAssetView* ModelLodAsset::Mesh::GetSemanticBufferAssetView(const AZ::Name& semantic) const
        {
            const AZStd::array_view<ModelLodAsset::Mesh::StreamBufferInfo>& streamBufferList = GetStreamBufferInfoList();

            for (const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : streamBufferList)
            {
                if (streamBufferInfo.m_semantic.m_name == semantic)
                {
                    return &streamBufferInfo.m_bufferAssetView;
                }
            }

            return nullptr;
        }

        AZStd::array_view<uint8_t> ModelLodAsset::Mesh::GetSemanticBuffer(const AZ::Name& semantic) const
        {
            if (const BufferAssetView* bufferAssetView = GetSemanticBufferAssetView(semantic))
            {
                if (const BufferAsset* bufferAsset = bufferAssetView->GetBufferAsset().Get())
                {
                    return bufferAsset->GetBuffer();
                }
            }

            return {};
        }

        void ModelLodAsset::SetReady()
        {
            m_status = Data::AssetData::AssetStatus::Ready;
        }
    } // namespace RPI
} // namespace AZ
