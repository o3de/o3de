/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ActorAsset.h>
#include <AtomActorInstance.h>

#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <MCore/Source/DualQuaternion.h>

// For creating a skinned mesh from an actor
#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Public/Model/Model.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/base.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/PackedVector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Component/Entity.h>

// Copied from ModelAssetBuilderComponent.cpp
namespace
{
    const uint32_t LinearSkinningFloatsPerBone = 12;
    const uint32_t DualQuaternionSkinningFloatsPerBone = 8;
    const uint32_t MaxSupportedSkinInfluences = 4;
}

namespace AZ
{
    namespace Render
    {
        static bool IsVertexCountWithinSupportedRange(size_t vertexOffset, size_t vertexCount)
        {
            return vertexOffset + vertexCount <= aznumeric_cast<size_t>(SkinnedMeshVertexStreamPropertyInterface::Get()->GetMaxSupportedVertexCount());
        }

        static void CalculateSubmeshPropertiesForLod(const Data::AssetId& actorAssetId, const EMotionFX::Actor* actor, size_t lodIndex, AZStd::vector<SkinnedSubMeshProperties>& subMeshes, uint32_t& lodIndexCount, uint32_t& lodVertexCount)
        {
            lodIndexCount = 0;
            lodVertexCount = 0;

            const Data::Asset<RPI::ModelLodAsset>& lodAsset = actor->GetMeshAsset()->GetLodAssets()[lodIndex];
            const AZStd::array_view<RPI::ModelLodAsset::Mesh> modelMeshes = lodAsset->GetMeshes();
            for (const RPI::ModelLodAsset::Mesh& modelMesh : modelMeshes)
            {
                const size_t subMeshIndexCount = modelMesh.GetIndexCount();
                const size_t subMeshVertexCount = modelMesh.GetVertexCount();
                if (subMeshVertexCount > 0)
                {
                    if (IsVertexCountWithinSupportedRange(lodVertexCount, subMeshVertexCount))
                    {
                        SkinnedSubMeshProperties skinnedSubMesh{};
                        skinnedSubMesh.m_indexOffset = lodIndexCount;
                        skinnedSubMesh.m_indexCount = aznumeric_cast<uint32_t>(subMeshIndexCount);
                        lodIndexCount += aznumeric_cast<uint32_t>(subMeshIndexCount);

                        skinnedSubMesh.m_vertexOffset = lodVertexCount;
                        skinnedSubMesh.m_vertexCount = aznumeric_cast<uint32_t>(subMeshVertexCount);
                        lodVertexCount += aznumeric_cast<uint32_t>(subMeshVertexCount);

                        skinnedSubMesh.m_materialSlot = actor->GetMeshAsset()->FindMaterialSlot(modelMesh.GetMaterialSlotId());

                        // Queue the material asset - the ModelLod seems to handle delayed material loads
                        skinnedSubMesh.m_materialSlot.m_defaultMaterialAsset.QueueLoad();

                        subMeshes.push_back(skinnedSubMesh);
                    }
                    else
                    {
                        AZStd::string assetPath;
                        Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &Data::AssetCatalogRequests::GetAssetPathById, actorAssetId);
                        AZ_Error("ActorAsset", false, "Lod '%d' for actor '%s' has greater than %d, the maximum supported number of vertices for a skinned sub-mesh. Sub-mesh will be ignored and not all vertices will be rendered.", lodIndex, assetPath.c_str(), SkinnedMeshVertexStreamPropertyInterface::Get()->GetMaxSupportedVertexCount());
                    }
                }
            }
        }

        static void ProcessIndicesForSubmesh(size_t indexCount, size_t atomIndexBufferOffset, size_t emfxSourceVertexStart, const uint32_t* emfxSubMeshIndices, AZStd::vector<uint32_t>& indexBufferData)
        {
            for (size_t index = 0; index < indexCount; ++index)
            {
                // The emfxSubMeshIndices is a pointer to the start of the indices for a particular sub-mesh, so we need to copy the indices from 0-indexCount instead of offsetting the start by emfxSourceVertexStart like we do with the other buffers
                // Also, the emfxSubMeshIndices are relative to the start vertex of the sub-mesh, so we need to subtract emfxSourceVertexStart to get the actual index of the vertex within the lod's vertex buffer
                indexBufferData[atomIndexBufferOffset + index] = emfxSubMeshIndices[index] - aznumeric_cast<uint32_t>(emfxSourceVertexStart);
            }
        }

        static void ProcessPositionsForSubmesh(size_t vertexCount, size_t atomVertexBufferOffset, size_t emfxSourceVertexStart, const AZ::Vector3* emfxSourcePositions, AZStd::vector<PackedVector3f>& positionBufferData, SkinnedSubMeshProperties& submesh)
        {
            // Pack the source Vector3 positions (which have 4 components under the hood) into a PackedVector3f buffer for Atom, and build an Aabb along the way
            // ATOM-3898 Investigate buffer format and alignment performance to compare current packed R32G32B32 buffer with R32G32B32A32 buffer
            Aabb localAabb = Aabb::CreateNull();
            for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                const Vector3& sourcePosition = emfxSourcePositions[emfxSourceVertexStart + vertexIndex];
                localAabb.AddPoint(sourcePosition);
                positionBufferData[atomVertexBufferOffset + vertexIndex] = PackedVector3f(sourcePosition);
            }

            submesh.m_aabb = localAabb;
        }

        static void ProcessNormalsForSubmesh(size_t vertexCount, size_t atomVertexBufferOffset, size_t emfxSourceVertexStart, const AZ::Vector3* emfxSourceNormals, AZStd::vector<PackedVector3f>& normalBufferData)
        {
            // Pack the source Vector3 normals (which have 4 components under the hood) into a PackedVector3f buffer for Atom
            // ATOM-3898 Investigate buffer format and alignment performance to compare current packed R32G32B32 buffer with R32G32B32A32 buffer
            for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                const Vector3& sourceNormal = emfxSourceNormals[emfxSourceVertexStart + vertexIndex];
                normalBufferData[atomVertexBufferOffset + vertexIndex] = PackedVector3f(sourceNormal);
            }
        }

        static void ProcessUVsForSubmesh(size_t vertexCount, size_t atomVertexBufferOffset, [[maybe_unused]] size_t emfxSourceVertexStart, const AZ::Vector2* emfxSourceUVs, AZStd::vector<float[2]>& uvBufferData)
        {
            for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                emfxSourceUVs[vertexIndex].StoreToFloat2(uvBufferData[atomVertexBufferOffset + vertexIndex]);
            }
        }

        static void ProcessTangentsForSubmesh(size_t vertexCount, size_t atomVertexBufferOffset, size_t emfxSourceVertexStart, const AZ::Vector4* emfxSourceTangents, AZStd::vector<Vector4>& tangentBufferData)
        {
            AZStd::copy(&emfxSourceTangents[emfxSourceVertexStart], &emfxSourceTangents[emfxSourceVertexStart + vertexCount], tangentBufferData.data() + atomVertexBufferOffset);
        }

        static void ProcessBitangentsForSubmesh(size_t vertexCount, size_t atomVertexBufferOffset, size_t emfxSourceVertexStart, const AZ::Vector3* emfxSourceBitangents, AZStd::vector<PackedVector3f>& bitangentBufferData)
        {
            AZ_Assert(emfxSourceBitangents, "GenerateBitangentsForSubmesh called with null source normals.");

            // Pack the source Vector3 bitangents (which have 4 components under the hood) into a PackedVector3f buffer for Atom
            // ATOM-3898 Investigate buffer format and alignment performance to compare current packed R32G32B32 buffer with R32G32B32A32 buffer
            for (size_t i = 0; i < vertexCount; ++i)
            {
                const Vector3& sourceBitangent = emfxSourceBitangents[emfxSourceVertexStart + i];
                bitangentBufferData[atomVertexBufferOffset + i] = PackedVector3f(sourceBitangent);
            }
        }

        static void GenerateBitangentsForSubmesh(size_t vertexCount, size_t atomVertexBufferOffset, size_t emfxSourceVertexStart, const AZ::Vector3* emfxSourceNormals, const AZ::Vector4* emfxSourceTangents, AZStd::vector<PackedVector3f>& bitangentBufferData)
        {
            AZ_Assert(emfxSourceNormals, "GenerateBitangentsForSubmesh called with null source normals.");
            AZ_Assert(emfxSourceTangents, "GenerateBitangentsForSubmesh called with null source tangents.");

            // Compute bitangent from tangent and normal.
            for (size_t i = 0; i < vertexCount; ++i)
            {
                const Vector4& sourceTangent = emfxSourceTangents[emfxSourceVertexStart + i];
                const Vector3& sourceNormal = emfxSourceNormals[emfxSourceVertexStart + i];
                const Vector3 bitangent = sourceNormal.Cross(sourceTangent.GetAsVector3()) * sourceTangent.GetW();
                bitangentBufferData[atomVertexBufferOffset + i] = PackedVector3f(bitangent);
            }
        }

        static void ProcessSkinInfluences(
            const EMotionFX::Mesh* mesh,
            const EMotionFX::SubMesh* subMesh,
            size_t atomVertexBufferOffset,
            AZStd::vector<uint32_t[MaxSupportedSkinInfluences / 2]>& blendIndexBufferData,
            AZStd::vector<AZStd::array<float, MaxSupportedSkinInfluences>>& blendWeightBufferData,
            bool hasClothData)
        {
            EMotionFX::SkinningInfoVertexAttributeLayer* sourceSkinningInfo = static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID));
            
            // EMotionFX source gives 16 bit indices and 32 bit float weights
            // Atom consumes 32 bit uint indices and 32 bit float weights (range 0-1)
            // Up to MaxSupportedSkinInfluences influences per vertex are supported
            const uint32_t* sourceOriginalVertex = static_cast<uint32_t*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));
            const uint32_t vertexCount = subMesh->GetNumVertices();
            const uint32_t vertexStart = subMesh->GetStartVertex();
            if (sourceSkinningInfo)
            {
                for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    const uint32_t originalVertex = sourceOriginalVertex[vertexIndex + vertexStart];
                    const uint32_t influenceCount = AZStd::GetMin<uint32_t>(MaxSupportedSkinInfluences, static_cast<uint32_t>(sourceSkinningInfo->GetNumInfluences(originalVertex)));
                    uint32_t influenceIndex = 0;

                    AZStd::vector<uint32_t> localIndices;
                    for (; influenceIndex < influenceCount; ++influenceIndex)
                    {
                        EMotionFX::SkinInfluence* influence = sourceSkinningInfo->GetInfluence(originalVertex, influenceIndex);
                        localIndices.push_back(static_cast<uint32_t>(influence->GetNodeNr()));
                        blendWeightBufferData[atomVertexBufferOffset + vertexIndex][influenceIndex] = influence->GetWeight();
                    }

                    // Zero out any unused ids/weights
                    for (; influenceIndex < MaxSupportedSkinInfluences; ++influenceIndex)
                    {
                        localIndices.push_back(0);
                        blendWeightBufferData[atomVertexBufferOffset + vertexIndex][influenceIndex] = 0.0f;
                    }

                    // Now that we have the 16-bit indices, pack them into 32-bit uints
                    for (size_t i = 0; i < localIndices.size(); ++i)
                    {
                        if (i % 2 == 0)
                        {
                            // Put the first/even ids in the most significant bits
                            blendIndexBufferData[atomVertexBufferOffset + vertexIndex][i / 2] = localIndices[i] << 16;
                        }
                        else
                        {
                            // Put the next/odd ids in the least significant bits
                            blendIndexBufferData[atomVertexBufferOffset + vertexIndex][i / 2] |= localIndices[i];
                        }
                    }
                }
            }

            // [TODO ATOM-15288]
            // Temporary workaround. If there is cloth data, set all the blend weights to zero to indicate
            // the vertices will be updated by cpu. When meshes with cloth data are not dispatched for skinning
            // this can be hasClothData can be removed.

            // If there is no skinning info, default to 0 weights and display an error
            if (hasClothData || !sourceSkinningInfo)
            {
                for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    for (uint32_t influenceIndex = 0; influenceIndex < MaxSupportedSkinInfluences; ++influenceIndex)
                    {
                        blendWeightBufferData[atomVertexBufferOffset + vertexIndex][influenceIndex] = 0.0f;
                    }
                }
            }
        }
        
        static void ProcessMorphsForLod(const EMotionFX::Actor* actor, const Data::Asset<RPI::BufferAsset>& morphBufferAsset, size_t lodIndex, const AZStd::string& fullFileName, SkinnedMeshInputLod& skinnedMeshLod)
        {
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodIndex);
            if (morphSetup)
            {
                AZ_Assert(actor->GetMorphTargetMetaAsset().IsReady(), "Trying to create morph targets from actor '%s', but the MorphTargetMetaAsset isn't loaded.", actor->GetName());
                const AZStd::vector<AZ::RPI::MorphTargetMetaAsset::MorphTarget>& metaDatas = actor->GetMorphTargetMetaAsset()->GetMorphTargets();

                // Loop over all the EMotionFX morph targets
                const size_t numMorphTargets = morphSetup->GetNumMorphTargets();
                for (size_t morphTargetIndex = 0; morphTargetIndex < numMorphTargets; ++morphTargetIndex)
                {
                    EMotionFX::MorphTargetStandard* morphTarget = static_cast<EMotionFX::MorphTargetStandard*>(morphSetup->GetMorphTarget(morphTargetIndex));
                    for (const auto& metaData : metaDatas)
                    {
                        // Loop through the metadatas to find the one that corresponds with the current morph target
                        // This ensures the order stays in sync with the order in the MorphSetup,
                        // so that the correct weights are applied to the correct morphs later
                        // Skip any that don't modify any vertices
                        if (metaData.m_morphTargetName == morphTarget->GetNameString() && metaData.m_numVertices > 0)
                        {
                            // The skinned mesh lod gets a unique morph for each meta, since each one has unique min/max delta values to use for decompression
                            const AZStd::string morphString = AZStd::string::format("%s_Lod%zu_Morph_%s", fullFileName.c_str(), lodIndex, metaData.m_meshNodeName.c_str());

                            float minWeight = morphTarget->GetRangeMin();
                            float maxWeight = morphTarget->GetRangeMax();

                            skinnedMeshLod.AddMorphTarget(metaData, morphBufferAsset, morphString, minWeight, maxWeight);
                        }
                    }
                }
            }
        }

        AZStd::intrusive_ptr<SkinnedMeshInputBuffers> CreateSkinnedMeshInputFromActor(const Data::AssetId& actorAssetId, const EMotionFX::Actor* actor)
        {

            Data::Asset<RPI::ModelAsset> modelAsset = actor->GetMeshAsset();
            if (!modelAsset.IsReady())
            {
                AZ_Warning("CreateSkinnedMeshInputFromActor", false, "Check if the actor has a mesh added. Right click the source file in the asset browser, click edit settings, "
                    "and navigate to the Meshes tab. Add a mesh if it's missing.");
                return nullptr;
            }

            AZStd::intrusive_ptr<SkinnedMeshInputBuffers> skinnedMeshInputBuffers = aznew SkinnedMeshInputBuffers;

            skinnedMeshInputBuffers->SetAssetId(actorAssetId);

            // Get the fileName, which will be used to label the buffers
            AZStd::string assetPath;
            Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &Data::AssetCatalogRequests::GetAssetPathById, actorAssetId);
            AZStd::string fullFileName;
            AzFramework::StringFunc::Path::GetFullFileName(assetPath.c_str(), fullFileName);

            // GetNumNodes returns the number of 'joints' or 'bones' in the skeleton
            const size_t numJoints = actor->GetNumNodes();
            const size_t numLODs = actor->GetNumLODLevels();

            // Create the containers to hold the data for all the combined sub-meshes
            AZStd::vector<uint32_t> indexBufferData;
            AZStd::vector<PackedVector3f> positionBufferData;
            AZStd::vector<PackedVector3f> normalBufferData;
            AZStd::vector<Vector4> tangentBufferData;
            AZStd::vector<PackedVector3f> bitangentBufferData;
            AZStd::vector<uint32_t[MaxSupportedSkinInfluences / 2]> blendIndexBufferData;
            AZStd::vector<AZStd::array<float, MaxSupportedSkinInfluences>> blendWeightBufferData;
            AZStd::vector<float[2]> uvBufferData;

            //
            // Process all LODs from the EMotionFX actor data.
            //

            skinnedMeshInputBuffers->SetLodCount(numLODs);
            AZ_Assert(numLODs == modelAsset->GetLodCount(), "The lod count of the EMotionFX mesh and Atom model are out of sync for '%s'", fullFileName.c_str());
            for (size_t lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                // Create a single LOD
                SkinnedMeshInputLod skinnedMeshLod;

                Data::Asset<RPI::ModelLodAsset> modelLodAsset = modelAsset->GetLodAssets()[lodIndex];

                // Each mesh vertex stream is packed into a single buffer for the whole lod. Get the first mesh, which can be used to retrieve the underlying buffer assets
                AZ_Assert(modelLodAsset->GetMeshes().size() > 0, "ModelLod '%d' for model '%s' has 0 meshes", lodIndex, fullFileName.c_str());
                const RPI::ModelLodAsset::Mesh& mesh0 = modelLodAsset->GetMeshes()[0];

                // Do a pass over the lod to find the number of sub-meshes, the offset and size of each sub-mesh, and total number of vertices in the lod.
                // These will be combined into one input buffer for the source actor, but these offsets and sizes will be used to create multiple sub-meshes for the target skinned actor
                uint32_t lodVertexCount = 0;
                uint32_t lodIndexCount = 0;
                AZStd::vector<SkinnedSubMeshProperties> subMeshes;
                CalculateSubmeshPropertiesForLod(actorAssetId, actor, lodIndex, subMeshes, lodIndexCount, lodVertexCount);

                skinnedMeshLod.SetIndexCount(lodIndexCount);
                skinnedMeshLod.SetVertexCount(lodVertexCount);

                // We'll be overwriting all the elements, so no need to construct them when resizing
                indexBufferData.resize_no_construct(lodIndexCount);
                positionBufferData.resize_no_construct(lodVertexCount);
                normalBufferData.resize_no_construct(lodVertexCount);
                tangentBufferData.resize_no_construct(lodVertexCount);
                bitangentBufferData.resize_no_construct(lodVertexCount);
                blendIndexBufferData.resize_no_construct(lodVertexCount);
                blendWeightBufferData.resize_no_construct(lodVertexCount);
                uvBufferData.resize_no_construct(lodVertexCount);

                // Now iterate over the actual data and populate the data for the per-actor buffers
                size_t indexBufferOffset = 0;
                size_t vertexBufferOffset = 0;
                size_t skinnedMeshSubmeshIndex = 0;
                for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex)
                {
                    const EMotionFX::Mesh* mesh = actor->GetMesh(static_cast<uint32>(lodIndex), static_cast<uint32>(jointIndex));
                    if (!mesh || mesh->GetIsCollisionMesh())
                    {
                        continue;
                    }

                    // Each of these is one long buffer containing the data for all sub-meshes in the joint
                    const AZ::Vector3* sourcePositions = static_cast<const AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
                    const AZ::Vector3* sourceNormals = static_cast<const AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));
                    const AZ::Vector4* sourceTangents = static_cast<const AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
                    const AZ::Vector3* sourceBitangents = static_cast<const AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_BITANGENTS));
                    const AZ::Vector2* sourceUVs = static_cast<const AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0));

                    const bool hasUVs = (sourceUVs != nullptr);
                    const bool hasTangents = (sourceTangents != nullptr);
                    const bool hasBitangents = (sourceBitangents != nullptr);

                    // For each sub-mesh within each mesh, we want to create a separate sub-piece.
                    const size_t numSubMeshes = mesh->GetNumSubMeshes();

                    AZ_Assert(numSubMeshes == modelLodAsset->GetMeshes().size(),
                        "Number of submeshes (%d) in EMotionFX mesh (lod %d and joint index %d) doesn't match the number of meshes (%d) in model lod asset",
                        numSubMeshes, lodIndex, jointIndex, modelLodAsset->GetMeshes().size());

                    for (size_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                    {
                        const EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(static_cast<uint32>(subMeshIndex));
                        const size_t vertexCount = subMesh->GetNumVertices();

                        // Skip empty sub-meshes and sub-meshes that would put the total vertex count beyond the supported range
                        if (vertexCount > 0 && IsVertexCountWithinSupportedRange(vertexBufferOffset, vertexCount))
                        {
                            const size_t indexCount = subMesh->GetNumIndices();
                            const uint32_t* indices = subMesh->GetIndices();
                            const size_t vertexStart = subMesh->GetStartVertex();

                            ProcessIndicesForSubmesh(indexCount, indexBufferOffset, vertexStart, indices, indexBufferData);
                            ProcessPositionsForSubmesh(vertexCount, vertexBufferOffset, vertexStart, sourcePositions, positionBufferData, subMeshes[skinnedMeshSubmeshIndex]);
                            ProcessNormalsForSubmesh(vertexCount, vertexBufferOffset, vertexStart, sourceNormals, normalBufferData);

                            AZ_Assert(hasUVs, "ActorAsset '%s' lod '%d' missing uvs. Downstream code is assuming all actors have uvs", fullFileName.c_str(), lodIndex);
                            if (hasUVs)
                            {
                                ProcessUVsForSubmesh(vertexCount, vertexBufferOffset, vertexStart, sourceUVs, uvBufferData);
                            }
                            // ATOM-3623 Support multiple UV sets in actors

                            // ATOM-3972 Support actors that don't have tangents
                            AZ_Assert(hasTangents, "ActorAsset '%s' lod '%d'  missing tangents. Downstream code is assuming all actors have tangents", fullFileName.c_str(), lodIndex);
                            if (hasTangents)
                            {
                                ProcessTangentsForSubmesh(vertexCount, vertexBufferOffset, vertexStart, sourceTangents, tangentBufferData);
                                if (hasBitangents)
                                {
                                    ProcessBitangentsForSubmesh(vertexCount, vertexBufferOffset, vertexStart, sourceBitangents, bitangentBufferData);
                                }
                                else
                                {
                                    GenerateBitangentsForSubmesh(vertexCount, vertexBufferOffset, vertexStart, sourceNormals, sourceTangents, bitangentBufferData);
                                }
                            }

                            // Check if the model mesh asset has cloth data. One ModelLodAsset::Mesh corresponds to one EMotionFX::SubMesh.
                            const bool hasClothData = modelLodAsset->GetMeshes()[subMeshIndex].GetSemanticBufferAssetView(AZ::Name("CLOTH_DATA")) != nullptr;

                            ProcessSkinInfluences(mesh, subMesh, vertexBufferOffset, blendIndexBufferData, blendWeightBufferData, hasClothData);

                            // Increment offsets so that the next sub-mesh can start at the right place
                            indexBufferOffset += indexCount;
                            vertexBufferOffset += vertexCount;
                            skinnedMeshSubmeshIndex++;
                        }
                    }   // for all submeshes
                } // for all meshes

                // Now that the data has been prepped, set the actual buffers
                skinnedMeshLod.SetModelLodAsset(modelLodAsset);

                // Set read-only buffers and views for input buffers that are shared across all instances
                AZStd::string lodString = AZStd::string::format("_Lod%zu", lodIndex);
                skinnedMeshLod.SetSkinningInputBufferAsset(mesh0.GetSemanticBufferAssetView(Name{ "POSITION" })->GetBufferAsset(), SkinnedMeshInputVertexStreams::Position);
                skinnedMeshLod.SetSkinningInputBufferAsset(mesh0.GetSemanticBufferAssetView(Name{ "NORMAL" })->GetBufferAsset(), SkinnedMeshInputVertexStreams::Normal);
                skinnedMeshLod.SetSkinningInputBufferAsset(mesh0.GetSemanticBufferAssetView(Name{ "TANGENT" })->GetBufferAsset(), SkinnedMeshInputVertexStreams::Tangent);
                skinnedMeshLod.SetSkinningInputBufferAsset(mesh0.GetSemanticBufferAssetView(Name{ "BITANGENT" })->GetBufferAsset(), SkinnedMeshInputVertexStreams::BiTangent);

                if (!mesh0.GetSemanticBufferAssetView(Name{ "SKIN_JOINTINDICES" }) || !mesh0.GetSemanticBufferAssetView(Name{ "SKIN_WEIGHTS" }))
                {
                    AZ_Error("ProcessSkinInfluences", false, "Actor '%s' lod '%zu' has no skin influences, and will be stuck in bind pose.", fullFileName.c_str(), lodIndex);
                }
                else
                {
                    Data::Asset<RPI::BufferAsset> jointIndicesBufferAsset = mesh0.GetSemanticBufferAssetView(Name{ "SKIN_JOINTINDICES" })->GetBufferAsset();
                    skinnedMeshLod.SetSkinningInputBufferAsset(jointIndicesBufferAsset, SkinnedMeshInputVertexStreams::BlendIndices);
                    Data::Asset<RPI::BufferAsset> skinWeightsBufferAsset = mesh0.GetSemanticBufferAssetView(Name{ "SKIN_WEIGHTS" })->GetBufferAsset();
                    skinnedMeshLod.SetSkinningInputBufferAsset(skinWeightsBufferAsset, SkinnedMeshInputVertexStreams::BlendWeights);

                    // We're using the indices/weights buffers directly from the model.
                    // However, EMFX has done some re-mapping of the id's, so we need to update the GPU buffer for it to have the correct data.
                    size_t remappedJointIndexBufferSizeInBytes = blendIndexBufferData.size() * sizeof(blendIndexBufferData[0]);
                    size_t remappedSkinWeightsBufferSizeInBytes = blendWeightBufferData.size() * sizeof(blendWeightBufferData[0]);

                    AZ_Assert(jointIndicesBufferAsset->GetBufferDescriptor().m_byteCount == remappedJointIndexBufferSizeInBytes, "Joint indices data from EMotionFX is not the same size as the buffer from the model in '%s', lod '%d'", fullFileName.c_str(), lodIndex);
                    AZ_Assert(skinWeightsBufferAsset->GetBufferDescriptor().m_byteCount == remappedSkinWeightsBufferSizeInBytes, "Skin weights data from EMotionFX is not the same size as the buffer from the model in '%s', lod '%d'", fullFileName.c_str(), lodIndex);

                    if (Data::Instance<RPI::Buffer> jointIndicesBuffer = RPI::Buffer::FindOrCreate(jointIndicesBufferAsset))
                    {
                        jointIndicesBuffer->UpdateData(blendIndexBufferData.data(), remappedJointIndexBufferSizeInBytes);
                    }
                    if (Data::Instance<RPI::Buffer> skinWeightsBuffer = RPI::Buffer::FindOrCreate(skinWeightsBufferAsset))
                    {
                        skinWeightsBuffer->UpdateData(blendWeightBufferData.data(), remappedSkinWeightsBufferSizeInBytes);
                    }
                }

                // Create read-only input assembly buffers that are not modified during skinning and shared across all instances
                skinnedMeshLod.SetIndexBufferAsset(mesh0.GetIndexBufferAssetView().GetBufferAsset());
                skinnedMeshLod.SetStaticBufferAsset(mesh0.GetSemanticBufferAssetView(Name{ "UV" })->GetBufferAsset(), SkinnedMeshStaticVertexStreams::UV_0);

                const RPI::BufferAssetView* morphBufferAssetView = nullptr;
                for (const auto& mesh : modelLodAsset->GetMeshes())
                {
                    morphBufferAssetView = mesh.GetSemanticBufferAssetView(Name{ "MORPHTARGET_VERTEXDELTAS" });
                    if (morphBufferAssetView)
                    {
                        break;
                    }
                }

                if (morphBufferAssetView)
                {
                    ProcessMorphsForLod(actor, morphBufferAssetView->GetBufferAsset(), static_cast<uint32_t>(lodIndex), fullFileName, skinnedMeshLod);
                }

                // Set colors after morphs are set, so that we know whether or not they are dynamic (if they exist)
                const RPI::BufferAssetView* colorView = mesh0.GetSemanticBufferAssetView(Name{ "COLOR" });
                if (colorView)
                {
                    if (skinnedMeshLod.HasDynamicColors())
                    {
                        // If colors are being morphed,
                        // add them as input to the skinning compute shader, which will apply the morph
                        skinnedMeshLod.SetSkinningInputBufferAsset(colorView->GetBufferAsset(), SkinnedMeshInputVertexStreams::Color);
                    }
                    else
                    {
                        // If colors exist but are not modified dynamically,
                        // add them to the static streams that are shared by all instances of the same skinned mesh
                        skinnedMeshLod.SetStaticBufferAsset(colorView->GetBufferAsset(), SkinnedMeshStaticVertexStreams::Color);
                    }
                }

                // Set the data that needs to be tracked on a per-sub-mesh basis
                // and create the common, shared sub-mesh buffer views
                skinnedMeshLod.SetSubMeshProperties(subMeshes);

                skinnedMeshInputBuffers->SetLod(lodIndex, skinnedMeshLod);

            } // for all lods

            return skinnedMeshInputBuffers;
        }

        void GetBoneTransformsFromActorInstance(const EMotionFX::ActorInstance* actorInstance, AZStd::vector<float>& boneTransforms, EMotionFX::Integration::SkinningMethod skinningMethod)
        {
            const EMotionFX::TransformData* transforms = actorInstance->GetTransformData();
            const AZ::Matrix3x4* skinningMatrices = transforms->GetSkinningMatrices();

            // For linear skinning, we need a 3x4 row-major float matrix for each transform
            const size_t numBoneTransforms = transforms->GetNumTransforms();

            if (skinningMethod == EMotionFX::Integration::SkinningMethod::Linear)
            {
                boneTransforms.resize_no_construct(numBoneTransforms * LinearSkinningFloatsPerBone);
                for (size_t i = 0; i < numBoneTransforms; ++i)
                {
                    skinningMatrices[i].StoreToRowMajorFloat12(&boneTransforms[i * LinearSkinningFloatsPerBone]);
                }
            }
            else if(skinningMethod == EMotionFX::Integration::SkinningMethod::DualQuat)
            {
                boneTransforms.resize_no_construct(numBoneTransforms * DualQuaternionSkinningFloatsPerBone);
                for (size_t i = 0; i < numBoneTransforms; ++i)
                {
                    MCore::DualQuaternion dualQuat = MCore::DualQuaternion::ConvertFromTransform(AZ::Transform::CreateFromMatrix3x4(skinningMatrices[i]));
                    dualQuat.m_real.StoreToFloat4(&boneTransforms[i * DualQuaternionSkinningFloatsPerBone]);
                    dualQuat.m_dual.StoreToFloat4(&boneTransforms[i * DualQuaternionSkinningFloatsPerBone + 4]);
                }
            }
        }

        Data::Instance<RPI::Buffer> CreateBoneTransformBufferFromActorInstance(const EMotionFX::ActorInstance* actorInstance, EMotionFX::Integration::SkinningMethod skinningMethod)
        {
            // Get the actual transforms
            AZStd::vector<float> boneTransforms;
            GetBoneTransformsFromActorInstance(actorInstance, boneTransforms, skinningMethod);

            uint32_t floatsPerBone = 0;
            if (skinningMethod == EMotionFX::Integration::SkinningMethod::Linear)
            {
                floatsPerBone = LinearSkinningFloatsPerBone;
            }
            else if (skinningMethod == EMotionFX::Integration::SkinningMethod::DualQuat)
            {
                floatsPerBone = DualQuaternionSkinningFloatsPerBone;
            }
            else
            {
                AZ_Error("ActorAsset", false, "Unsupported EMotionFX skinning method.");
            }

            // Create a buffer and populate it with the transforms
            RPI::CommonBufferDescriptor descriptor;
            descriptor.m_bufferData = boneTransforms.data();
            descriptor.m_bufferName = AZStd::string::format("BoneTransformBuffer_%s", actorInstance->GetActor()->GetName());
            descriptor.m_byteCount = boneTransforms.size() * sizeof(float);
            descriptor.m_elementSize = static_cast<uint32_t>(floatsPerBone * sizeof(float));
            descriptor.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
            return RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(descriptor);
        }

    } //namespace Render
} // namespace AZ
