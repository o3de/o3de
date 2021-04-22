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

#include <Cry_Math.h> // Needed for DualQuat
#include <MathConversion.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <Integration/ActorComponentBus.h>

 // Needed to access the Mesh information inside Actor.
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/ActorInstance.h>

#include <Components/ClothComponentMesh/ActorClothSkinning.h>
#include <Utils/AssetHelper.h>

#include <AzCore/Math/PackedVector3.h>

namespace NvCloth
{
    namespace Internal
    {
        bool ObtainSkinningData(
            AZ::EntityId entityId, 
            const MeshNodeInfo& meshNodeInfo,
            const size_t numSimParticles,
            const AZStd::vector<int>& meshRemappedVertices,
            AZStd::vector<SkinningInfo>& skinningData)
        {
            AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
            AZ::Render::MeshComponentRequestBus::EventResult(
                modelAsset, entityId, &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);
            if (!modelAsset.IsReady())
            {
                return false;
            }

            if (modelAsset->GetLodCount() < meshNodeInfo.m_lodLevel)
            {
                return false;
            }

            const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& modelLodAsset = modelAsset->GetLodAssets()[meshNodeInfo.m_lodLevel];
            if (!modelLodAsset.GetId().IsValid())
            {
                return false;
            }

            EMotionFX::ActorInstance* actorInstance = nullptr;
            EMotionFX::Integration::ActorComponentRequestBus::EventResult(actorInstance, entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
            if (!actorInstance)
            {
                return false;
            }

            const EMotionFX::Actor* actor = actorInstance->GetActor();
            if (!actor)
            {
                return false;
            }

            const auto& skinToSkeletonIndexMap = actor->GetSkinToSkeletonIndexMap();

            skinningData.resize(numSimParticles);

            // For each submesh...
            for (const auto& subMeshInfo : meshNodeInfo.m_subMeshes)
            {
                if (modelLodAsset->GetMeshes().size() < subMeshInfo.m_primitiveIndex)
                {
                    AZ_Error("ActorClothSkinning", false,
                        "Unable to access submesh %d from lod asset '%s' as it only has %d submeshes.",
                        subMeshInfo.m_primitiveIndex,
                        modelAsset.GetHint().c_str(),
                        modelLodAsset->GetMeshes().size());
                    return false;
                }

                const AZ::RPI::ModelLodAsset::Mesh& subMesh = modelLodAsset->GetMeshes()[subMeshInfo.m_primitiveIndex];

                const auto sourcePositions = subMesh.GetSemanticBufferTyped<AZ::PackedVector3f>(AZ::Name("POSITION"));
                if (sourcePositions.size() != subMeshInfo.m_numVertices)
                {
                    AZ_Error("ActorClothSkinning", false,
                        "Number of vertices (%zu) in submesh %d doesn't match the cloth's submesh (%d)",
                        sourcePositions.size(), subMeshInfo.m_primitiveIndex, subMeshInfo.m_numVertices);
                    return false;
                }

                const auto sourceSkinJointIndices = subMesh.GetSemanticBufferTyped<uint16_t>(AZ::Name("SKIN_JOINTINDICES"));
                const auto sourceSkinWeights = subMesh.GetSemanticBufferTyped<float>(AZ::Name("SKIN_WEIGHTS"));

                if (sourceSkinJointIndices.empty() || sourceSkinWeights.empty())
                {
                    continue;
                }
                AZ_Assert(sourceSkinJointIndices.size() == sourceSkinWeights.size(),
                    "Size of skin joint indices buffer (%zu) different from skin weights buffer (%zu)",
                    sourceSkinJointIndices.size(), sourceSkinWeights.size());

                const size_t influenceCount = sourceSkinWeights.size() / sourcePositions.size();
                if (influenceCount == 0)
                {
                    continue;
                }

                for (int index = 0; index < subMeshInfo.m_numVertices; ++index)
                {
                    const int skinnedDataIndex = meshRemappedVertices[subMeshInfo.m_verticesFirstIndex + index];
                    if (skinnedDataIndex < 0)
                    {
                        // Removed particle
                        continue;
                    }

                    SkinningInfo& skinningInfo = skinningData[skinnedDataIndex];
                    skinningInfo.m_jointIndices.resize(influenceCount);
                    skinningInfo.m_jointWeights.resize(influenceCount);

                    for (size_t influenceIndex = 0; influenceIndex < influenceCount; ++influenceIndex)
                    {
                        const AZ::u16 jointIndex = sourceSkinJointIndices[index * influenceCount + influenceIndex];
                        const float weight = sourceSkinWeights[index * influenceCount + influenceIndex];

                        auto skeletonIndexIt = skinToSkeletonIndexMap.find(jointIndex);
                        if (skeletonIndexIt == skinToSkeletonIndexMap.end())
                        {
                            AZ_Error("ActorClothSkinning", false,
                                "Joint index %d from model asset not found in map to skeleton indices",
                                jointIndex);
                            return false;
                        }

                        skinningInfo.m_jointIndices[influenceIndex] = skeletonIndexIt->second;
                        skinningInfo.m_jointWeights[influenceIndex] = weight;
                    }
                }
            }

            return true;
        }

        EMotionFX::Integration::SkinningMethod ObtainSkinningMethod(AZ::EntityId entityId)
        {
            EMotionFX::Integration::SkinningMethod skinningMethod =
                EMotionFX::Integration::SkinningMethod::DualQuat;

            EMotionFX::Integration::ActorComponentRequestBus::EventResult(skinningMethod, entityId,
                &EMotionFX::Integration::ActorComponentRequestBus::Events::GetSkinningMethod);

            return skinningMethod;
        }

        const AZ::Matrix3x4* ObtainSkinningMatrices(AZ::EntityId entityId)
        {
            EMotionFX::ActorInstance* actorInstance = nullptr;
            EMotionFX::Integration::ActorComponentRequestBus::EventResult(actorInstance, entityId, 
                &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
            if (!actorInstance)
            {
                return nullptr;
            }

            const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
            if (!transformData)
            {
                return nullptr;
            }

            return transformData->GetSkinningMatrices();
        }

        AZStd::unordered_map<AZ::u16, DualQuat> ObtainSkinningDualQuaternions(
            AZ::EntityId entityId,
            const AZStd::vector<AZ::u16>& jointIndices)
        {
            const AZ::Matrix3x4* skinningMatrices = ObtainSkinningMatrices(entityId);
            if (!skinningMatrices)
            {
                return {};
            }

            AZStd::unordered_map<AZ::u16, DualQuat> skinningDualQuaternions;
            for (AZ::u16 jointIndex : jointIndices)
            {
                skinningDualQuaternions.emplace(jointIndex, AZMatrix3x4ToLYMatrix3x4(skinningMatrices[jointIndex]));
            }
            return skinningDualQuaternions;
        }
    }

    // Specialized class that applies linear blending skinning
    class ActorClothSkinningLinear
        : public ActorClothSkinning
    {
    public:
        explicit ActorClothSkinningLinear(AZ::EntityId entityId)
            : ActorClothSkinning(entityId)
        {
        }

        // ActorClothSkinning overrides ...
        void UpdateSkinning() override;
        void ApplySkinning(
            const AZStd::vector<AZ::Vector4>& originalPositions,
            AZStd::vector<AZ::Vector4>& positions) override;

    private:
        AZ::Vector3 ComputeSkinnedPosition(
            const AZ::Vector3& originalPosition,
            const SkinningInfo& skinningInfo,
            const AZ::Matrix3x4* skinningMatrices);

        const AZ::Matrix3x4* m_skinningMatrices = nullptr;
    };

    void ActorClothSkinningLinear::UpdateSkinning()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        m_skinningMatrices = Internal::ObtainSkinningMatrices(m_entityId);
    }

    void ActorClothSkinningLinear::ApplySkinning(
        const AZStd::vector<AZ::Vector4>& originalPositions,
        AZStd::vector<AZ::Vector4>& positions)
    {
        if (!m_skinningMatrices)
        {
            return;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        for (size_t index = 0; index < originalPositions.size(); ++index)
        {
            const AZ::Vector3 skinnedPosition = ComputeSkinnedPosition(
                originalPositions[index].GetAsVector3(),
                m_skinningData[index],
                m_skinningMatrices);

            // Avoid overwriting the w component
            positions[index].Set(skinnedPosition, positions[index].GetW());
        }
    }

    AZ::Vector3 ActorClothSkinningLinear::ComputeSkinnedPosition(
        const AZ::Vector3& originalPosition,
        const SkinningInfo& skinningInfo,
        const AZ::Matrix3x4* skinningMatrices)
    {
        AZ::Matrix3x4 clothSkinningMatrix = AZ::Matrix3x4::CreateZero();
        for (size_t weightIndex = 0; weightIndex < skinningInfo.m_jointWeights.size(); ++weightIndex)
        {
            const AZ::u16 jointIndex = skinningInfo.m_jointIndices[weightIndex];
            const float jointWeight = skinningInfo.m_jointWeights[weightIndex];

            if (AZ::IsClose(jointWeight, 0.0f))
            {
                continue;
            }

            // Blending matrices the same way done in GPU shaders, by adding each weighted matrix element by element.
            // This way the skinning results are much similar to the skinning performed in GPU.
            for (int i = 0; i < 3; ++i)
            {
                clothSkinningMatrix.SetRow(i, clothSkinningMatrix.GetRow(i) + skinningMatrices[jointIndex].GetRow(i) * jointWeight);
            }
        }

        return clothSkinningMatrix * originalPosition;
    }

    // Specialized class that applies dual quaternion blending skinning
    class ActorClothSkinningDualQuaternion
        : public ActorClothSkinning
    {
    public:
        explicit ActorClothSkinningDualQuaternion(AZ::EntityId entityId)
            : ActorClothSkinning(entityId)
        {
        }

        // ActorClothSkinning overrides ...
        void UpdateSkinning() override;
        void ApplySkinning(
            const AZStd::vector<AZ::Vector4>& originalPositions,
            AZStd::vector<AZ::Vector4>& positions) override;

    private:
        AZ::Vector3 ComputeSkinnedPosition(
            const AZ::Vector3& originalPosition,
            const SkinningInfo& skinningInfo,
            const AZStd::unordered_map<AZ::u16, DualQuat>& skinningDualQuaternions);

        AZStd::unordered_map<AZ::u16, DualQuat> m_skinningDualQuaternions;
    };


    void ActorClothSkinningDualQuaternion::UpdateSkinning()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        m_skinningDualQuaternions = Internal::ObtainSkinningDualQuaternions(m_entityId, m_jointIndices);
    }

    void ActorClothSkinningDualQuaternion::ApplySkinning(
        const AZStd::vector<AZ::Vector4>& originalPositions,
        AZStd::vector<AZ::Vector4>& positions)
    {
        if (m_skinningDualQuaternions.empty())
        {
            return;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        for (size_t index = 0; index < originalPositions.size(); ++index)
        {
            const AZ::Vector3 skinnedPosition = ComputeSkinnedPosition(
                originalPositions[index].GetAsVector3(),
                m_skinningData[index],
                m_skinningDualQuaternions);

            // Avoid overwriting the w component
            positions[index].Set(skinnedPosition, positions[index].GetW());
        }
    }

    AZ::Vector3 ActorClothSkinningDualQuaternion::ComputeSkinnedPosition(
        const AZ::Vector3& originalPosition,
        const SkinningInfo& skinningInfo,
        const AZStd::unordered_map<AZ::u16, DualQuat>& skinningDualQuaternions)
    {
        DualQuat clothSkinningDualQuaternion(type_zero::ZERO);
        for (size_t weightIndex = 0; weightIndex < skinningInfo.m_jointWeights.size(); ++weightIndex)
        {
            const AZ::u16 jointIndex = skinningInfo.m_jointIndices[weightIndex];
            const float jointWeight = skinningInfo.m_jointWeights[weightIndex];

            if (AZ::IsClose(jointWeight, 0.0f))
            {
                continue;
            }

            clothSkinningDualQuaternion += skinningDualQuaternions.at(jointIndex) * jointWeight;
        }
        clothSkinningDualQuaternion.Normalize();

        return LYVec3ToAZVec3(clothSkinningDualQuaternion * AZVec3ToLYVec3(originalPosition));
    }

    AZStd::unique_ptr<ActorClothSkinning> ActorClothSkinning::Create(
        AZ::EntityId entityId, 
        const MeshNodeInfo& meshNodeInfo,
        const size_t numSimParticles,
        const AZStd::vector<int>& meshRemappedVertices)
    {
        AZStd::vector<SkinningInfo> skinningData;
        if (!Internal::ObtainSkinningData(entityId, meshNodeInfo, numSimParticles, meshRemappedVertices, skinningData))
        {
            return nullptr;
        }

        if (numSimParticles != skinningData.size())
        {
            AZ_Error("ActorClothSkinning", false, 
                "Number of simulation particles (%zu) doesn't match with skinning data obtained (%zu)", 
                numSimParticles, skinningData.size());
            return nullptr;
        }

        AZStd::unique_ptr<ActorClothSkinning> actorClothSkinning;
        const auto skinningMethod = Internal::ObtainSkinningMethod(entityId);
        switch (skinningMethod)
        {
        case EMotionFX::Integration::SkinningMethod::DualQuat:
            actorClothSkinning = AZStd::make_unique<ActorClothSkinningDualQuaternion>(entityId);
            break;

        case EMotionFX::Integration::SkinningMethod::Linear:
            actorClothSkinning = AZStd::make_unique<ActorClothSkinningLinear>(entityId);
            break;

        default:
            AZ_Error("ActorClothSkinning", false,
                "Unknown skinning method (%u).", static_cast<AZ::u32>(skinningMethod));
            return nullptr;
        }

        // Insert the indices of the joints that influence the particle (weight is not 0)
        AZStd::set<AZ::u16> jointIndices;
        for (size_t particleIndex = 0; particleIndex < numSimParticles; ++particleIndex)
        {
            const auto& skinningInfo = skinningData[particleIndex];
            for (size_t weightIndex = 0; weightIndex < skinningInfo.m_jointWeights.size(); ++weightIndex)
            {
                const AZ::u16 jointIndex = skinningInfo.m_jointIndices[weightIndex];
                const float jointWeight = skinningInfo.m_jointWeights[weightIndex];

                if (AZ::IsClose(jointWeight, 0.0f))
                {
                    continue;
                }

                jointIndices.insert(jointIndex);
            }
        }
        actorClothSkinning->m_jointIndices.assign(jointIndices.begin(), jointIndices.end());

        actorClothSkinning->m_skinningData = AZStd::move(skinningData);

        return actorClothSkinning;
    }

    ActorClothSkinning::ActorClothSkinning(AZ::EntityId entityId)
        : m_entityId(entityId)
    {
    }

    void ActorClothSkinning::UpdateActorVisibility()
    {
        bool isVisible = true;

        EMotionFX::ActorInstance* actorInstance = nullptr;
        EMotionFX::Integration::ActorComponentRequestBus::EventResult(actorInstance, m_entityId,
            &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
        if (actorInstance)
        {
            isVisible = actorInstance->GetIsVisible();
        }

        m_wasActorVisible = m_isActorVisible;
        m_isActorVisible = isVisible;
    }

    bool ActorClothSkinning::IsActorVisible() const
    {
        return m_isActorVisible;
    }

    bool ActorClothSkinning::WasActorVisible() const
    {
        return m_wasActorVisible;
    }
} // namespace NvCloth
