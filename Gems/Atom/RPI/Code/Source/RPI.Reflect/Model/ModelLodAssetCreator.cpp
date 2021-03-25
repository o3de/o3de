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

#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace RPI
    {
        void ModelLodAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void ModelLodAssetCreator::SetLodIndexBuffer(const Data::Asset<BufferAsset>& bufferAsset)
        {
            if (ValidateIsReady())
            {
                m_asset->m_indexBuffer = AZStd::move(bufferAsset);
            }
        }

        void ModelLodAssetCreator::AddLodStreamBuffer(const Data::Asset<BufferAsset>& bufferAsset)
        {
            if (ValidateIsReady())
            {
                m_asset->m_streamBuffers.push_back(AZStd::move(bufferAsset));
            }
        }

        void ModelLodAssetCreator::BeginMesh()
        {
            if (ValidateIsReady())
            {
                m_currentMesh = ModelLodAsset::Mesh();
                m_meshBegan = true;
            }
        }

        void ModelLodAssetCreator::SetMeshName(const AZ::Name& name)
        {
            if (ValidateIsMeshReady())
            {
                m_currentMesh.m_name = name;
            }
        }

        void ModelLodAssetCreator::SetMeshAabb(AZ::Aabb&& aabb)
        {
            if (ValidateIsMeshReady())
            {
                m_currentMesh.m_aabb = AZStd::move(aabb);
            }
        }

        void ModelLodAssetCreator::SetMeshMaterialAsset(const Data::Asset<MaterialAsset>& materialAsset)
        {
            if (ValidateIsMeshReady())
            {
                m_currentMesh.m_materialAsset = materialAsset;
            }
        }

        void ModelLodAssetCreator::SetMeshIndexBuffer(const BufferAssetView& bufferAssetView)
        {
            if (!ValidateIsMeshReady())
            {
                return;
            }

            if (m_currentMesh.m_indexBufferAssetView.GetBufferAsset().Get() != nullptr)
            {
                ReportError("The current mesh has already had an index buffer set.");
                return;
            }

            m_currentMesh.m_indexBufferAssetView = AZStd::move(bufferAssetView);
        }

        bool ModelLodAssetCreator::AddMeshStreamBuffer(
            const RHI::ShaderSemantic& streamSemantic,
            const AZ::Name& customName,
            const BufferAssetView& bufferAssetView)
        {
            if (!ValidateIsMeshReady())
            {
                return false;
            }

            if (m_currentMesh.m_streamBufferInfo.size() >= RHI::Limits::Pipeline::StreamCountMax)
            {
                ReportError("Cannot add another stream buffer info. Maximum of %d already reached.", RHI::Limits::Pipeline::StreamCountMax);
                return false;
            }

            // If this streamId already exists throw an error
            for (const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : m_currentMesh.m_streamBufferInfo)
            {
                if (streamBufferInfo.m_semantic == streamSemantic || (!streamBufferInfo.m_customName.IsEmpty() && streamBufferInfo.m_customName == customName))
                {
                    ReportError("Failed to add Stream Buffer. Buffer with this streamId or name already exists.");
                    return false;
                }
            }
            
            ModelLodAsset::Mesh::StreamBufferInfo streamBufferInfo;
            streamBufferInfo.m_semantic = streamSemantic;
            streamBufferInfo.m_customName = customName;
            streamBufferInfo.m_bufferAssetView = AZStd::move(bufferAssetView);

            m_currentMesh.m_streamBufferInfo.push_back(AZStd::move(streamBufferInfo));

            return true;
        }

        void ModelLodAssetCreator::AddMeshStreamBuffer(
            const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo)
        {
            if (!ValidateIsMeshReady())
            {
                return;
            }

            // If this semantic already exists throw an error
            for (const ModelLodAsset::Mesh::StreamBufferInfo& existingInfo : m_currentMesh.m_streamBufferInfo)
            {
                if (existingInfo.m_semantic == streamBufferInfo.m_semantic || (!existingInfo.m_customName.IsEmpty() && existingInfo.m_customName == streamBufferInfo.m_customName))
                {
                    ReportError("Failed to add Stream Buffer. Buffer with this semantic or name already exists.");
                    return;
                }
            }

            m_currentMesh.m_streamBufferInfo.push_back(AZStd::move(streamBufferInfo));
        }

        void ModelLodAssetCreator::EndMesh()
        {
            if (ValidateIsMeshReady() && ValidateMesh(m_currentMesh))
            {
                m_asset->AddMesh(AZStd::move(m_currentMesh));
                m_meshBegan = false;
            }
        }

        bool ModelLodAssetCreator::End(Data::Asset<ModelLodAsset>& result)
        {
            if (ValidateIsReady() && ValidateIsMeshEnded() && ValidateLod())
            {
                m_asset->SetReady();
                return EndCommon(result);
            }

            return false;
        }


        bool ModelLodAssetCreator::ValidateIsMeshReady()
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (!m_meshBegan)
            {
                AZ_Assert(false, "BeginMesh() was not called");
                return false;
            }

            return true;
        }

        bool ModelLodAssetCreator::ValidateIsMeshEnded()
        {
            if (m_meshBegan)
            {
                AZ_Assert(false, "MeshEnd() was not called");
                return false;
            }

            return true;
        }

        bool ModelLodAssetCreator::ValidateLod()
        {
            if (m_asset->GetMeshes().empty())
            {
                ReportError("No meshes have been provided for this LOD");
                return false;
            }

            return true;
        }

        bool ModelLodAssetCreator::ValidateMesh(const ModelLodAsset::Mesh& mesh)
        {
            if (mesh.GetVertexCount() == 0)
            {
                ReportError("Mesh has a vertex count of 0");
                return false;
            }

            if (mesh.GetIndexCount() == 0)
            {
                ReportError("Mesh has an index count of 0");
                return false;
            }

            if (!mesh.GetAabb().IsValid())
            {
                ReportError("Mesh does not have a valid Aabb");
                return false;
            }

            if (mesh.m_indexBufferAssetView.GetBufferAsset().Get() == nullptr)
            {
                ReportError("Mesh does not have a valid index buffer");
                return false;
            }

            for (const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : mesh.m_streamBufferInfo)
            {
                if (streamBufferInfo.m_bufferAssetView.GetBufferAsset().Get() == nullptr)
                {
                    ReportError("Mesh has an invalid stream buffer");
                    return false;
                }
            }

            return true;
        }
    } // namespace RPI
} // namespace AZ
