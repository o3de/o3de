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

#include <Integration/ActorComponentBus.h>

// Needed to access the Mesh information inside Actor.
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/ActorInstance.h>

#include <Components/ClothComponentMesh/ActorClothSkinning.h>

namespace NvCloth
{
    namespace Internal
    {
        bool ObtainSkinningData(
            AZ::EntityId entityId, 
            const AZStd::string& meshNode, 
            const size_t numSimParticles,
            const AZStd::vector<int>& meshRemappedVertices,
            AZStd::vector<SkinningInfo>& skinningData)
        {
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

            const uint32 numNodes = actor->GetNumNodes();
            const uint32 numLODs = actor->GetNumLODLevels();

            const EMotionFX::Mesh* emfxMesh = nullptr;

            // Find the render data of the mesh node
            for (uint32 lodLevel = 0; lodLevel < numLODs; ++lodLevel)
            {
                for (uint32 nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
                {
                    const EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, nodeIndex);
                    if (!mesh || mesh->GetIsCollisionMesh())
                    {
                        // Skip invalid and collision meshes.
                        continue;
                    }

                    const EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodeIndex);
                    if (meshNode != node->GetNameString())
                    {
                        // Skip nodes other than the one we're looking for.
                        continue;
                    }

                    emfxMesh = mesh;
                    break;
                }

                if (emfxMesh)
                {
                    break;
                }
            }

            if (!emfxMesh)
            {
                return false;
            }

            const AZ::u32* sourceOriginalVertex = static_cast<AZ::u32*>(emfxMesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));
            EMotionFX::SkinningInfoVertexAttributeLayer* sourceSkinningInfo = 
                static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(
                    emfxMesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID));

            if (!sourceOriginalVertex || !sourceSkinningInfo)
            {
                return false;
            }

            const int numVertices = emfxMesh->GetNumVertices();
            if (numVertices == 0)
            {
                AZ_Error("ActorClothSkinning", false, "Invalid mesh data");
                return false;
            }

            if (meshRemappedVertices.size() != numVertices)
            {
                AZ_Error("ActorClothSkinning", false,
                    "Number of vertices (%d) doesn't match the mesh remapping size (%zu)",
                    numVertices, meshRemappedVertices.size());
                return false;
            }

            skinningData.resize(numSimParticles);
            for (int index = 0; index < numVertices; ++index)
            {
                const int skinnedDataIndex = meshRemappedVertices[index];
                if (skinnedDataIndex < 0)
                {
                    // Removed particle
                    continue;
                }

                SkinningInfo& skinningInfo = skinningData[skinnedDataIndex];

                const AZ::u32 originalVertex = sourceOriginalVertex[index];
                const AZ::u32 influenceCount = AZ::GetMin<AZ::u32>(MaxSkinningBones, sourceSkinningInfo->GetNumInfluences(originalVertex));
                AZ::u32 influenceIndex = 0;
                AZ::u8 weightError = 255;

                for (; influenceIndex < influenceCount; ++influenceIndex)
                {
                    EMotionFX::SkinInfluence* influence = sourceSkinningInfo->GetInfluence(originalVertex, influenceIndex);
                    skinningInfo.m_jointIndices[influenceIndex] = influence->GetNodeNr();
                    skinningInfo.m_jointWeights[influenceIndex] = static_cast<AZ::u8>(AZ::GetClamp<float>(influence->GetWeight() * 255.0f, 0.0f, 255.0f));
                    if (skinningInfo.m_jointWeights[influenceIndex] >= weightError)
                    {
                        skinningInfo.m_jointWeights[influenceIndex] = weightError;
                        weightError = 0;
                        influenceIndex++;
                        break;
                    }
                    else
                    {
                        weightError -= skinningInfo.m_jointWeights[influenceIndex];
                    }
                }

                skinningInfo.m_jointWeights[0] += weightError;

                for (; influenceIndex < MaxSkinningBones; ++influenceIndex)
                {
                    skinningInfo.m_jointIndices[influenceIndex] = 0;
                    skinningInfo.m_jointWeights[influenceIndex] = 0;
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
        for (int weightIndex = 0; weightIndex < MaxSkinningBones; ++weightIndex)
        {
            if (skinningInfo.m_jointWeights[weightIndex] == 0)
            {
                continue;
            }

            const AZ::u16 jointIndex = skinningInfo.m_jointIndices[weightIndex];
            const float jointWeight = skinningInfo.m_jointWeights[weightIndex] / 255.0f;

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
        for (int weightIndex = 0; weightIndex < MaxSkinningBones; ++weightIndex)
        {
            if (skinningInfo.m_jointWeights[weightIndex] == 0)
            {
                continue;
            }

            const AZ::u16 jointIndex = skinningInfo.m_jointIndices[weightIndex];
            const float jointWeight = skinningInfo.m_jointWeights[weightIndex] / 255.0f;

            clothSkinningDualQuaternion += skinningDualQuaternions.at(jointIndex) * jointWeight;
        }
        clothSkinningDualQuaternion.Normalize();

        return LYVec3ToAZVec3(clothSkinningDualQuaternion * AZVec3ToLYVec3(originalPosition));
    }

    AZStd::unique_ptr<ActorClothSkinning> ActorClothSkinning::Create(
        AZ::EntityId entityId, 
        const AZStd::string& meshNode,
        const size_t numSimParticles,
        const AZStd::vector<int>& meshRemappedVertices)
    {
        AZStd::vector<SkinningInfo> skinningData;
        if (!Internal::ObtainSkinningData(entityId, meshNode, numSimParticles, meshRemappedVertices, skinningData))
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
            for (int weightIndex = 0; weightIndex < MaxSkinningBones; ++weightIndex)
            {
                if (skinningData[particleIndex].m_jointWeights[weightIndex] == 0)
                {
                    continue;
                }

                const AZ::u16 jointIndex = skinningData[particleIndex].m_jointIndices[weightIndex];
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
