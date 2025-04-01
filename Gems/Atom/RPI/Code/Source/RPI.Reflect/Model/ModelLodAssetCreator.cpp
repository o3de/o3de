/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/set.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
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

        void ModelLodAssetCreator::SetMeshAabb(const AZ::Aabb& aabb)
        {
            if (ValidateIsMeshReady())
            {
                m_currentMesh.m_aabb = aabb;
            }
        }
                
        void ModelLodAssetCreator::SetMeshMaterialSlot(ModelMaterialSlot::StableId id)
        {
            if (!ValidateIsMeshReady())
            {
                return;
            }

            m_currentMesh.m_materialSlotId = id;
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

        bool ModelLodAssetCreator::Clone(const Data::Asset<ModelLodAsset>& sourceAsset, Data::Asset<ModelLodAsset>& clonedResult, Data::AssetId& inOutLastCreatedAssetId)
        {
            AZStd::span<const ModelLodAsset::Mesh> sourceMeshes = sourceAsset->GetMeshes();
            if (sourceMeshes.empty())
            {
                return true;
            }

            ModelLodAssetCreator creator;
            inOutLastCreatedAssetId.m_subId = inOutLastCreatedAssetId.m_subId + 1;
            creator.Begin(inOutLastCreatedAssetId);

            // Add the index buffer
            const Data::Asset<BufferAsset> sourceIndexBufferAsset = sourceMeshes[0].GetIndexBufferAssetView().GetBufferAsset();
            Data::Asset<BufferAsset> clonedIndexBufferAsset;
            BufferAssetCreator::Clone(sourceIndexBufferAsset, clonedIndexBufferAsset, inOutLastCreatedAssetId);
            creator.SetLodIndexBuffer(clonedIndexBufferAsset);

            // Add meshes
            AZStd::unordered_map<AZ::Data::AssetId, Data::Asset<BufferAsset>> oldToNewBufferAssets;
            for (const ModelLodAsset::Mesh& sourceMesh : sourceMeshes)
            {
                // Add stream buffers
                for (const AZ::RPI::ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : sourceMesh.GetStreamBufferInfoList())
                {
                    const Data::Asset<BufferAsset>& sourceStreamBuffer = streamBufferInfo.m_bufferAssetView.GetBufferAsset();
                    const AZ::Data::AssetId sourceBufferAssetId = sourceStreamBuffer.GetId();

                    // In case the buffer asset id is not part of our old to new asset id mapping, we did not convert and add it yet.
                    if (oldToNewBufferAssets.find(sourceBufferAssetId) == oldToNewBufferAssets.end())
                    {
                        Data::Asset<BufferAsset> streamBufferAsset;
                        if (!BufferAssetCreator::Clone(sourceStreamBuffer, streamBufferAsset, inOutLastCreatedAssetId))
                        {
                            AZ_Error("ModelLodAssetCreator", false,
                                "Cannot clone buffer asset for '%s'.", sourceBufferAssetId.ToString<AZStd::string>().c_str());
                            return false;
                        }

                        oldToNewBufferAssets[sourceBufferAssetId] = streamBufferAsset;
                        creator.AddLodStreamBuffer(streamBufferAsset);
                    }
                }

                // Add mesh
                creator.BeginMesh();
                creator.SetMeshName(sourceMesh.GetName());
                AZ::Aabb aabb = sourceMesh.GetAabb();
                creator.SetMeshAabb(aabb);

                creator.SetMeshMaterialSlot(sourceMesh.GetMaterialSlotId());

                // Mesh index buffer view
                const BufferAssetView& sourceIndexBufferView = sourceMesh.GetIndexBufferAssetView();
                BufferAssetView indexBufferAssetView(clonedIndexBufferAsset, sourceIndexBufferView.GetBufferViewDescriptor());
                creator.SetMeshIndexBuffer(indexBufferAssetView);

                // Mesh stream buffer views
                for (const AZ::RPI::ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : sourceMesh.GetStreamBufferInfoList())
                {
                    // Get the corresponding new buffer asset id from the source buffer.
                    const AZ::Data::AssetId sourceBufferAssetId = streamBufferInfo.m_bufferAssetView.GetBufferAsset().GetId();
                    const auto assetIdIterator = oldToNewBufferAssets.find(sourceBufferAssetId);
                    if (assetIdIterator != oldToNewBufferAssets.end())
                    {
                        const Data::Asset<BufferAsset>& clonedBufferAsset = assetIdIterator->second;
                        BufferAssetView bufferAssetView(clonedBufferAsset, streamBufferInfo.m_bufferAssetView.GetBufferViewDescriptor());
                        creator.AddMeshStreamBuffer(streamBufferInfo.m_semantic, streamBufferInfo.m_customName, bufferAssetView);
                    }
                    else
                    {
                        AZ_Error("ModelLodAssetCreator", false,
                            "Cannot find cloned buffer asset for source buffer asset '%s'.",
                            sourceBufferAssetId.ToString<AZStd::string>().c_str());
                        return false;
                    }
                }

                creator.EndMesh();
            }

            return creator.End(clonedResult);
        }
    } // namespace RPI
} // namespace AZ
