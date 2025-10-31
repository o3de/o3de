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
#include <Atom/RPI.Reflect/Model/ModelAssetHelpers.h>
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

#include <inttypes.h>

// Copied from ModelAssetBuilderComponent.cpp
namespace
{
    const uint32_t LinearSkinningFloatsPerBone = 12;
    const uint32_t DualQuaternionSkinningFloatsPerBone = 8;
}

namespace AZ
{
    namespace Render
    {
        static bool IsVertexCountWithinSupportedRange(size_t vertexOffset, size_t vertexCount)
        {
            return vertexOffset + vertexCount <= aznumeric_cast<size_t>(SkinnedMeshVertexStreamPropertyInterface::Get()->GetMaxSupportedVertexCount());
        }

        static void ProcessSkinInfluences(
            const EMotionFX::Mesh* mesh,
            const EMotionFX::SubMesh* subMesh,
            const uint32_t maxInfluencesPerVertex,
            AZStd::vector<uint32_t>& blendIndexBufferData,
            AZStd::vector<float>& blendWeightBufferData)
        {
            EMotionFX::SkinningInfoVertexAttributeLayer* sourceSkinningInfo = static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID));

            // EMotionFX source gives 16 bit indices and 32 bit float weights
            // Atom consumes 32 bit uint indices and 32 bit float weights (range 0-1)
            const uint32_t* sourceOriginalVertex = static_cast<uint32_t*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));
            const uint32_t vertexCount = subMesh->GetNumVertices();
            const uint32_t vertexStart = subMesh->GetStartVertex();
            if (sourceSkinningInfo)
            {
                for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    const uint32_t originalVertex = sourceOriginalVertex[vertexIndex + vertexStart];
                    const uint32_t influenceCount = AZStd::GetMin<uint32_t>(
                        maxInfluencesPerVertex, static_cast<uint32_t>(sourceSkinningInfo->GetNumInfluences(originalVertex)));

                    uint32_t influenceIndex = 0;
                    for (; influenceIndex < influenceCount; ++influenceIndex)
                    {
                        EMotionFX::SkinInfluence* influence = sourceSkinningInfo->GetInfluence(originalVertex, influenceIndex);
                        // Pack the 16-bit indices into 32-bit uints, putting the first of each index pair in the most significant bits
                        if (influenceIndex % 2 == 0)
                        {
                            blendIndexBufferData.push_back(static_cast<uint32_t>(influence->GetNodeNr()) << 16);
                        }
                        else
                        {
                            blendIndexBufferData.back() |= static_cast<uint32_t>(influence->GetNodeNr());
                        }
                        blendWeightBufferData.push_back(influence->GetWeight());
                    }

                    // Zero out any unused ids/weights
                    for (; influenceIndex < maxInfluencesPerVertex; ++influenceIndex)
                    {
                        if (influenceIndex % 2 == 0)
                        {
                            blendIndexBufferData.push_back(0);
                        }
                        blendWeightBufferData.push_back(0.0f);
                    }
                }

                //Pad the blend weight and index buffers in order for it to respect alignment of RPI::SkinnedMeshBufferAlignment
                //as that is the expected behavior of the source asset
                RPI::ModelAssetHelpers::AlignStreamBuffer<float>(
                    blendWeightBufferData, blendWeightBufferData.size(), RPI::SkinWeightFormat, RPI::SkinnedMeshBufferAlignment);

                //We dont use RPI::SkinIndicesFormat as blendIndexBufferData contains uint32_t instead of uint16_t
                RHI::Format blendIndexFormat = RHI::Format::R32_FLOAT;
                RPI::ModelAssetHelpers::AlignStreamBuffer<uint32_t>(
                    blendIndexBufferData, blendIndexBufferData.size(), blendIndexFormat, RPI::SkinnedMeshBufferAlignment);
            }
        }

        static void ProcessMorphsForLod(
            uint32_t lodIndex,
            const EMotionFX::Actor* actor,
            const AZStd::string& fullFileName,
            AZStd::intrusive_ptr<SkinnedMeshInputBuffers> skinnedMeshInputBuffers)
        {
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodIndex);
            if (morphSetup)
            {
                const auto& modelLodAsset = skinnedMeshInputBuffers->GetLod(lodIndex).GetModelLodAsset();

                AZ_Assert(actor->GetMorphTargetMetaAsset().IsReady(), "Trying to create morph targets from actor '%s', but the MorphTargetMetaAsset isn't loaded.", actor->GetName());
                const AZStd::vector<AZ::RPI::MorphTargetMetaAsset::MorphTarget>& metaDatas = actor->GetMorphTargetMetaAsset()->GetMorphTargets();

                // Loop over all the EMotionFX morph targets
                const size_t numMorphTargets = morphSetup->GetNumMorphTargets();
                for (size_t morphTargetIndex = 0; morphTargetIndex < numMorphTargets; ++morphTargetIndex)
                {
                    EMotionFX::MorphTargetStandard* morphTarget = static_cast<EMotionFX::MorphTargetStandard*>(morphSetup->GetMorphTarget(morphTargetIndex));
                    for (const auto& metaData : metaDatas)
                    {
                        // Loop through the metadatas to find any that correspond with the current morph target.
                        // There may be more than one, since a single morph target may be distributed across multiple meshes.
                        // This ensures the order stays in sync with the order in the MorphSetup,
                        // so that the correct weights are applied to the correct morphs later
                        // Skip any that don't modify any vertices
                        if (metaData.m_morphTargetName == morphTarget->GetNameString() && metaData.m_numVertices > 0)
                        {
                            // The skinned mesh lod gets a unique morph for each meta, since each one has unique min/max delta values to use for decompression
                            const AZStd::string morphString = AZStd::string::format(
                                "%s_Lod%" PRIu32 "_Morph_%s", fullFileName.c_str(), lodIndex, metaData.m_meshNodeName.c_str());

                            float minWeight = morphTarget->GetRangeMin();
                            float maxWeight = morphTarget->GetRangeMax();

                            const auto lodMeshes = modelLodAsset->GetMeshes();
                            const auto& modelLodMesh = lodMeshes[metaData.m_meshIndex];

                            const RPI::BufferAssetView* morphBufferAssetView =
                                modelLodMesh.GetSemanticBufferAssetView(Name{ "MORPHTARGET_VERTEXDELTAS" });

                            skinnedMeshInputBuffers->AddMorphTarget(
                                lodIndex, metaData, morphBufferAssetView, morphString, minWeight, maxWeight);
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
            skinnedMeshInputBuffers->CreateFromModelAsset(modelAsset);

            // Get the fileName, which will be used to label the buffers
            AZStd::string assetPath;
            Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &Data::AssetCatalogRequests::GetAssetPathById, actorAssetId);
            AZStd::string fullFileName;
            AzFramework::StringFunc::Path::GetFullFileName(assetPath.c_str(), fullFileName);

            // GetNumNodes returns the number of 'joints' or 'bones' in the skeleton
            const size_t numJoints = actor->GetNumNodes();
            const size_t numLODs = actor->GetNumLODLevels();

            // Create the containers to hold the data for all the combined sub-meshes
            AZStd::vector<uint32_t> blendIndexBufferData;
            AZStd::vector<float> blendWeightBufferData;

            //
            // Process all LODs from the EMotionFX actor data.
            //
            AZ_Assert(numLODs == modelAsset->GetLodCount(), "The lod count of the EMotionFX mesh and Atom model are out of sync for '%s'", fullFileName.c_str());
            for (uint32_t lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                // Create a single LOD
                Data::Asset<RPI::ModelLodAsset> modelLodAsset = modelAsset->GetLodAssets()[lodIndex];

                // Clear out the vector for re-mapped joint data that will be populated by values from EMotionFX
                blendIndexBufferData.clear();
                blendWeightBufferData.clear();

                const RPI::BufferAssetView* indicesBuffAssetView =
                    modelLodAsset->GetSemanticBufferAssetView(Name(RPI::ShaderSemanticName_SkinJointIndices));
                const RPI::BufferAssetView* weightsBuffAssetView =
                    modelLodAsset->GetSemanticBufferAssetView(Name(RPI::ShaderSemanticName_SkinWeights));

                if(!indicesBuffAssetView || !weightsBuffAssetView)
                {
                    AZ_Warning(
                            "ProcessSkinInfluences", false,
                            "Actor '%s' lod '%" PRIu32 "' has no skin indice buffer, skinning would not be applicable on this mesh.", fullFileName.c_str(),
                            lodIndex);
                }
                else
                {
                    // Reserve enough memory for the default/common case. Use the element count from the main source buffer
                    blendIndexBufferData.reserve(indicesBuffAssetView->GetBufferAsset()->GetBufferViewDescriptor().m_elementCount);
                    blendWeightBufferData.reserve(weightsBuffAssetView->GetBufferAsset()->GetBufferViewDescriptor().m_elementCount);

                    // Now iterate over the actual data and populate the data for the per-actor buffers
                    uint32_t vertexBufferOffset = 0;
                    for (uint32_t jointIndex = 0; jointIndex < numJoints; ++jointIndex)
                    {
                        const EMotionFX::Mesh* mesh = actor->GetMesh(lodIndex, jointIndex);
                        if (!mesh || mesh->GetIsCollisionMesh())
                        {
                            continue;
                        }

                        // For each sub-mesh within each mesh, we want to create a separate sub-piece.
                        const uint32_t numSubMeshes = aznumeric_caster(mesh->GetNumSubMeshes());

                        AZ_Assert(
                            numSubMeshes == modelLodAsset->GetMeshes().size(),
                            "Number of submeshes (%" PRIu32 ") in EMotionFX mesh (lod %d and joint index %d) "
                            "doesn't match the number of meshes (%d) in model lod asset",
                            numSubMeshes, lodIndex, jointIndex, modelLodAsset->GetMeshes().size());

                        for (uint32_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                        {
                            const EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(static_cast<uint32>(subMeshIndex));
                            const uint32_t vertexCount = subMesh->GetNumVertices();

                            // Skip empty sub-meshes and sub-meshes that would put the total vertex count beyond the supported range
                            if (vertexCount > 0 && IsVertexCountWithinSupportedRange(vertexBufferOffset, vertexCount))
                            {
                                ProcessSkinInfluences(mesh, subMesh, skinnedMeshInputBuffers->GetInfluenceCountPerVertex(lodIndex, subMeshIndex), blendIndexBufferData, blendWeightBufferData);

                                // Increment offsets so that the next sub-mesh can start at the right place
                                vertexBufferOffset += vertexCount;
                            }
                        }   // for all submeshes
                    } // for all meshes

                    const RPI::BufferAssetView* jointIndicesBufferView = nullptr;
                    const RPI::BufferAssetView* skinWeightsBufferView = nullptr;

                    for (const auto& modelLodMesh : modelLodAsset->GetMeshes())
                    {
                        // TODO: operate on a per-mesh basis

                        // If the joint id/weight buffers haven't been found on a mesh yet, keep looking
                        if (!jointIndicesBufferView)
                        {
                            jointIndicesBufferView = modelLodMesh.GetSemanticBufferAssetView(Name{ "SKIN_JOINTINDICES" });
                            if (jointIndicesBufferView)
                            {
                                skinWeightsBufferView = modelLodMesh.GetSemanticBufferAssetView(Name{ "SKIN_WEIGHTS" });
                                AZ_Error("CreateSkinnedMeshInputFromActor", skinWeightsBufferView, "Mesh '%s' on actor '%s' has joint indices but no joint weights", modelLodMesh.GetName().GetCStr(), fullFileName.c_str());
                                break;
                            }
                        }
                    }

                    if (!jointIndicesBufferView || !skinWeightsBufferView)
                    {
                        AZ_Error(
                            "ProcessSkinInfluences", false,
                            "Actor '%s' lod '%" PRIu32 "' has no skin influences, and will be stuck in bind pose.", fullFileName.c_str(),
                            lodIndex);
                    }
                    else
                    {
                        Data::Asset<RPI::BufferAsset> jointIndicesBufferAsset = jointIndicesBufferView->GetBufferAsset();
                        Data::Asset<RPI::BufferAsset> skinWeightsBufferAsset = skinWeightsBufferView->GetBufferAsset();

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
                }

                ProcessMorphsForLod(lodIndex, actor, fullFileName, skinnedMeshInputBuffers);
            } // for all lods

            skinnedMeshInputBuffers->Finalize();
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
            else if (skinningMethod == EMotionFX::Integration::SkinningMethod::None)
            {
                AZ_Warning("ActorAsset", false, "Create bone transform called with no skinning, will return nullptr.");
                return nullptr;
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
