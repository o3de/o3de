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

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <Integration/ActorComponentBus.h>

// Needed to access the Mesh information inside Actor.
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <MCore/Source/DualQuaternion.h>

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
            const size_t numVertices,
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

            skinningData.resize(numVertices);

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

                for (int vertexIndex = 0; vertexIndex < subMeshInfo.m_numVertices; ++vertexIndex)
                {
                    SkinningInfo& skinningInfo = skinningData[subMeshInfo.m_verticesFirstIndex + vertexIndex];
                    skinningInfo.m_jointIndices.resize(influenceCount);
                    skinningInfo.m_jointWeights.resize(influenceCount);

                    for (size_t influenceIndex = 0; influenceIndex < influenceCount; ++influenceIndex)
                    {
                        const AZ::u16 jointIndex = sourceSkinJointIndices[vertexIndex * influenceCount + influenceIndex];
                        const float weight = sourceSkinWeights[vertexIndex * influenceCount + influenceIndex];

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

        AZStd::unordered_map<AZ::u16, MCore::DualQuaternion> ObtainSkinningDualQuaternions(
            AZ::EntityId entityId,
            const AZStd::vector<AZ::u16>& jointIndices)
        {
            const AZ::Matrix3x4* skinningMatrices = ObtainSkinningMatrices(entityId);
            if (!skinningMatrices)
            {
                return {};
            }

            AZStd::unordered_map<AZ::u16, MCore::DualQuaternion> skinningDualQuaternions;
            for (AZ::u16 jointIndex : jointIndices)
            {
                skinningDualQuaternions.emplace(jointIndex, MCore::DualQuaternion(AZ::Transform::CreateFromMatrix3x4(skinningMatrices[jointIndex])));
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
        void ApplySkinninOnNonSimulatedVertices(
            const MeshClothInfo& originalData,
            ClothComponentMesh::RenderData& renderData) override;

    private:
        AZ::Matrix3x4 ComputeVertexSkinnningTransform(const SkinningInfo& skinningInfo);

        const AZ::Matrix3x4* m_skinningMatrices = nullptr;

        inline static const AZ::Matrix3x4 s_zeroMatrix3x4 = AZ::Matrix3x4::CreateZero();
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
        if (!m_skinningMatrices ||
            originalPositions.empty() ||
            originalPositions.size() != positions.size() ||
            originalPositions.size() != m_simulatedVertices.size())
        {
            return;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        const size_t vertexCount = m_simulatedVertices.size();
        for (size_t index = 0; index < vertexCount; ++index)
        {
            const AZ::Matrix3x4 vertexSkinningTransform = ComputeVertexSkinnningTransform(m_skinningData[m_simulatedVertices[index]]);

            const AZ::Vector3 skinnedPosition = vertexSkinningTransform * originalPositions[index].GetAsVector3();
            positions[index].Set(skinnedPosition, positions[index].GetW()); // Avoid overwriting the w component
        }
    }

    void ActorClothSkinningLinear::ApplySkinninOnNonSimulatedVertices(
        const MeshClothInfo& originalData,
        ClothComponentMesh::RenderData& renderData)
    {
        if (!m_skinningMatrices ||
            originalData.m_particles.empty() ||
            originalData.m_particles.size() != renderData.m_particles.size() ||
            originalData.m_particles.size() != m_skinningData.size())
        {
            return;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        for (const AZ::u32 index : m_nonSimulatedVertices)
        {
            const AZ::Matrix3x4 vertexSkinningTransform = ComputeVertexSkinnningTransform(m_skinningData[index]);

            const AZ::Vector3 skinnedPosition = vertexSkinningTransform * originalData.m_particles[index].GetAsVector3();
            renderData.m_particles[index].Set(skinnedPosition, renderData.m_particles[index].GetW()); // Avoid overwriting the w component

            // Calculate the reciprocal scale version of the matrix to transform the vectors.
            const AZ::Matrix3x4 vertexSkinningTransformReciprocalScale = vertexSkinningTransform.GetReciprocalScaled();

            renderData.m_tangents[index] = vertexSkinningTransformReciprocalScale.TransformVector(originalData.m_tangents[index]).GetNormalized();
            renderData.m_bitangents[index] = vertexSkinningTransformReciprocalScale.TransformVector(originalData.m_bitangents[index]).GetNormalized();
            renderData.m_normals[index] = vertexSkinningTransformReciprocalScale.TransformVector(originalData.m_normals[index]).GetNormalized();
        }
    }

    AZ::Matrix3x4 ActorClothSkinningLinear::ComputeVertexSkinnningTransform(const SkinningInfo& skinningInfo)
    {
        AZ::Matrix3x4 vertexSkinningTransform = s_zeroMatrix3x4;
        const size_t jointWeightsCount = skinningInfo.m_jointWeights.size();
        for (size_t weightIndex = 0; weightIndex < jointWeightsCount; ++weightIndex)
        {
            const AZ::u16 jointIndex = skinningInfo.m_jointIndices[weightIndex];
            const float jointWeight = skinningInfo.m_jointWeights[weightIndex];

            // Blending matrices the same way done in GPU shaders, by adding each weighted matrix element by element.
            // This way the skinning results are much similar to the skinning performed in GPU.
            vertexSkinningTransform += m_skinningMatrices[jointIndex] * jointWeight;
        }
        return vertexSkinningTransform;
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
        void ApplySkinninOnNonSimulatedVertices(
            const MeshClothInfo& originalData,
            ClothComponentMesh::RenderData& renderData) override;

    private:
        MCore::DualQuaternion ComputeVertexSkinnningTransform(const SkinningInfo& skinningInfo);

        AZStd::unordered_map<AZ::u16, MCore::DualQuaternion> m_skinningDualQuaternions;

        inline static const MCore::DualQuaternion s_zeroDualQuaternion = MCore::DualQuaternion(AZ::Quaternion::CreateZero(), AZ::Quaternion::CreateZero());
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
        if (m_skinningDualQuaternions.empty() ||
            originalPositions.empty() ||
            originalPositions.size() != positions.size() ||
            originalPositions.size() != m_simulatedVertices.size())
        {
            return;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        const size_t vertexCount = m_simulatedVertices.size();
        for (size_t index = 0; index < vertexCount; ++index)
        {
            const MCore::DualQuaternion vertexSkinningTransform = ComputeVertexSkinnningTransform(m_skinningData[m_simulatedVertices[index]]);

            const AZ::Vector3 skinnedPosition = vertexSkinningTransform.TransformPoint(originalPositions[index].GetAsVector3());
            positions[index].Set(skinnedPosition, positions[index].GetW()); // Avoid overwriting the w component
        }
    }

    void ActorClothSkinningDualQuaternion::ApplySkinninOnNonSimulatedVertices(
        const MeshClothInfo& originalData,
        ClothComponentMesh::RenderData& renderData)
    {
        if (m_skinningDualQuaternions.empty() ||
            originalData.m_particles.empty() ||
            originalData.m_particles.size() != renderData.m_particles.size() ||
            originalData.m_particles.size() != m_skinningData.size())
        {
            return;
        }

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Cloth);

        for (const AZ::u32 index : m_nonSimulatedVertices)
        {
            const MCore::DualQuaternion vertexSkinningTransform = ComputeVertexSkinnningTransform(m_skinningData[index]);

            const AZ::Vector3 skinnedPosition = vertexSkinningTransform.TransformPoint(originalData.m_particles[index].GetAsVector3());
            renderData.m_particles[index].Set(skinnedPosition, renderData.m_particles[index].GetW()); // Avoid overwriting the w component

            // ComputeVertexSkinnningTransform is normalizing the dual quaternion, so it won't have scale
            // and there is no need to compute the reciprocal scale version for transforming vectors.

            renderData.m_tangents[index] = vertexSkinningTransform.TransformVector(originalData.m_tangents[index]).GetNormalized();
            renderData.m_bitangents[index] = vertexSkinningTransform.TransformVector(originalData.m_bitangents[index]).GetNormalized();
            renderData.m_normals[index] = vertexSkinningTransform.TransformVector(originalData.m_normals[index]).GetNormalized();
        }
    }

    MCore::DualQuaternion ActorClothSkinningDualQuaternion::ComputeVertexSkinnningTransform(const SkinningInfo& skinningInfo)
    {
        MCore::DualQuaternion vertexSkinningTransform = s_zeroDualQuaternion;
        const size_t jointWeightsCount = skinningInfo.m_jointWeights.size();
        for (size_t weightIndex = 0; weightIndex < jointWeightsCount; ++weightIndex)
        {
            const AZ::u16 jointIndex = skinningInfo.m_jointIndices[weightIndex];
            const float jointWeight = skinningInfo.m_jointWeights[weightIndex];

            const MCore::DualQuaternion& skinningDualQuaternion = m_skinningDualQuaternions.at(jointIndex);

            float flip = AZ::GetSign(vertexSkinningTransform.mReal.Dot(skinningDualQuaternion.mReal));
            vertexSkinningTransform += skinningDualQuaternion * jointWeight * flip;
        }
        // Normalizing the dual quaternion as the GPU shaders do. This will remove the scale from the transform.
        vertexSkinningTransform.Normalize();
        return vertexSkinningTransform;
    }

    AZStd::unique_ptr<ActorClothSkinning> ActorClothSkinning::Create(
        AZ::EntityId entityId, 
        const MeshNodeInfo& meshNodeInfo,
        const size_t numVertices,
        const size_t numSimulatedVertices,
        const AZStd::vector<int>& meshRemappedVertices)
    {
        AZStd::vector<SkinningInfo> skinningData;
        if (!Internal::ObtainSkinningData(entityId, meshNodeInfo, numVertices, skinningData))
        {
            return nullptr;
        }

        if (numVertices != skinningData.size())
        {
            AZ_Error("ActorClothSkinning", false, 
                "Number of vertices (%zu) doesn't match with skinning data obtained (%zu)", 
                numVertices, skinningData.size());
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

        // Collect all indices of the joints that influence the vertices
        AZStd::set<AZ::u16> jointIndices;
        for (size_t vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
        {
            const SkinningInfo& skinningInfo = skinningData[vertexIndex];
            const size_t jointIndicesCount = skinningInfo.m_jointIndices.size();
            for (size_t jointIndex = 0; jointIndex < jointIndicesCount; ++jointIndex)
            {
                jointIndices.insert(skinningInfo.m_jointIndices[jointIndex]);
            }
        }
        actorClothSkinning->m_jointIndices.assign(jointIndices.begin(), jointIndices.end());

        // Collect the indices for simulated and non-simulated vertices
        actorClothSkinning->m_simulatedVertices.resize(numSimulatedVertices);
        actorClothSkinning->m_nonSimulatedVertices.reserve(numVertices);
        for (size_t vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex)
        {
            const int remappedIndex = meshRemappedVertices[vertexIndex];
            if (remappedIndex >= 0)
            {
                actorClothSkinning->m_simulatedVertices[remappedIndex] = vertexIndex;
            }
            else
            {
                actorClothSkinning->m_nonSimulatedVertices.emplace_back(vertexIndex);
            }
        }
        actorClothSkinning->m_nonSimulatedVertices.shrink_to_fit();

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
