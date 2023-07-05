/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        bool ObtainSkinningInfluences(
            AZ::EntityId entityId,
            const MeshNodeInfo& meshNodeInfo,
            AZStd::vector<SkinningInfluence>& skinningInfluences,
            AZStd::vector<AZ::u32>& numInfluencesPerSubmesh)
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

            const auto modelLodAssets = modelAsset->GetLodAssets();
            const AZ::Data::Asset<AZ::RPI::ModelLodAsset> modelLodAsset = modelLodAssets[meshNodeInfo.m_lodLevel];
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

            // Pre-calculate the total number of vertices and the number of influences per-vertex
            size_t totalInfluenceCount = 0;
            numInfluencesPerSubmesh.clear();
            numInfluencesPerSubmesh.reserve(meshNodeInfo.m_subMeshes.size());
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

                const auto subMeshes = modelLodAsset->GetMeshes();
                const AZ::RPI::ModelLodAsset::Mesh& subMesh = subMeshes[subMeshInfo.m_primitiveIndex];

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
                    // Ignoring skinning when there is no skin data.
                    // All submeshes will either have or not have skin data, since they come from the same mesh.
                    return false;
                }
                AZ_Assert(sourceSkinJointIndices.size() == sourceSkinWeights.size(),
                    "Size of skin joint indices buffer (%zu) different from skin weights buffer (%zu)",
                    sourceSkinJointIndices.size(), sourceSkinWeights.size());

                const AZ::u32 subMeshInfluenceCount = aznumeric_caster(sourceSkinWeights.size() / sourcePositions.size());
                AZ_Assert(subMeshInfluenceCount > 0,
                    "Submesh %d skinning data has zero joint influences per vertex.",
                    subMeshInfo.m_primitiveIndex);

                numInfluencesPerSubmesh.push_back(subMeshInfluenceCount);
                totalInfluenceCount += sourceSkinWeights.size();
            }

            skinningInfluences.resize(totalInfluenceCount);

            // For each submesh...
            size_t currentInfluenceIndex = 0;
            for (size_t meshIndex = 0; meshIndex < meshNodeInfo.m_subMeshes.size(); ++meshIndex)
            {
                const auto& subMeshInfo = meshNodeInfo.m_subMeshes[meshIndex];
                const auto subMeshes = modelLodAsset->GetMeshes();
                const AZ::RPI::ModelLodAsset::Mesh& subMesh = subMeshes[subMeshInfo.m_primitiveIndex];
                const auto sourceSkinJointIndices = subMesh.GetSemanticBufferTyped<uint16_t>(AZ::Name("SKIN_JOINTINDICES"));
                const auto sourceSkinWeights = subMesh.GetSemanticBufferTyped<float>(AZ::Name("SKIN_WEIGHTS"));

                size_t numberOfInfluencesPerSubmesh = aznumeric_caster(numInfluencesPerSubmesh[meshIndex]);

                for (int subMeshVertexIndex = 0; subMeshVertexIndex < subMeshInfo.m_numVertices; ++subMeshVertexIndex)
                {
                    // sourceSkinJointIndices and sourceSkinWeights buffers are views into the influences for the current mesh.
                    // Get the offset to the start of the influences for the current vertex
                    const size_t influenceStart = subMeshVertexIndex * numberOfInfluencesPerSubmesh;

                    for (size_t influenceIndex = 0; influenceIndex < numberOfInfluencesPerSubmesh; ++influenceIndex)
                    {
                        const AZ::u16 jointIndex = sourceSkinJointIndices[influenceStart + influenceIndex];
                        const float weight = sourceSkinWeights[influenceStart + influenceIndex];

                        auto skeletonIndexIt = skinToSkeletonIndexMap.find(jointIndex);
                        if (skeletonIndexIt == skinToSkeletonIndexMap.end())
                        {
                            AZ_Error("ActorClothSkinning", false,
                                "Joint index %d from model asset not found in map to skeleton indices",
                                jointIndex);
                            return false;
                        }

                        // currentInfluenceIndex is the offset into the unified buffer of skin influences
                        // that combines all sub-meshes
                        skinningInfluences[currentInfluenceIndex].m_jointIndex = skeletonIndexIt->second;
                        skinningInfluences[currentInfluenceIndex].m_jointWeight = weight;
                        ++currentInfluenceIndex;
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
        void ApplySkinningOnNonSimulatedVertices(
            const MeshClothInfo& originalData,
            ClothComponentMesh::RenderData& renderData) override;

    private:
        AZ::Matrix3x4 ComputeVertexSkinningTransform(
            AZ::u32 influenceOffset, AZ::u32 numberOfInfluencesPerVertex);

        const AZ::Matrix3x4* m_skinningMatrices = nullptr;

        inline static const AZ::Matrix3x4 s_zeroMatrix3x4 = AZ::Matrix3x4::CreateZero();
    };

    void ActorClothSkinningLinear::UpdateSkinning()
    {
        AZ_PROFILE_FUNCTION(Cloth);

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

        AZ_PROFILE_FUNCTION(Cloth);

        const size_t vertexCount = m_simulatedVertices.size();
        for (size_t index = 0; index < vertexCount; ++index)
        {
            const SimulatedVertex& vertex = m_simulatedVertices[index];
            const AZ::Matrix3x4 vertexSkinningTransform = ComputeVertexSkinningTransform(vertex.m_influenceOffset, vertex.m_influenceCount);

            const AZ::Vector3 skinnedPosition = vertexSkinningTransform * originalPositions[index].GetAsVector3();
            positions[index].Set(skinnedPosition, positions[index].GetW()); // Avoid overwriting the w component
        }
    }

    void ActorClothSkinningLinear::ApplySkinningOnNonSimulatedVertices(
        const MeshClothInfo& originalData,
        ClothComponentMesh::RenderData& renderData)
    {
        if (!m_skinningMatrices ||
            originalData.m_particles.empty() ||
            originalData.m_particles.size() != renderData.m_particles.size())
        {
            return;
        }

        AZ_PROFILE_FUNCTION(Cloth);

        for (const NonSimulatedVertex& vertex : m_nonSimulatedVertices)
        {
            const AZ::u32 index = vertex.m_originalVertexIndex;
            const AZ::Matrix3x4 vertexSkinningTransform =
                ComputeVertexSkinningTransform(vertex.m_influenceOffset, vertex.m_influenceCount);

            const AZ::Vector3 skinnedPosition = vertexSkinningTransform * originalData.m_particles[index].GetAsVector3();
            renderData.m_particles[index].Set(skinnedPosition, renderData.m_particles[index].GetW()); // Avoid overwriting the w component

            // Calculate the reciprocal scale version of the matrix to transform the normals.
            // Note: This operation is not strictly equivalent to the full inverse transpose when the matrix's
            //       basis vectors are not perpendicular, which is the case blending linearly the matrices.
            //       This is a fast approximation, which is also done by the GPU skinning shader.
            const AZ::Matrix3x4 vertexSkinningTransformReciprocalScale = vertexSkinningTransform.GetReciprocalScaled();

            renderData.m_normals[index] = vertexSkinningTransformReciprocalScale.TransformVector(originalData.m_normals[index]).GetNormalized();

            // Tangents and Bitangents are recalculated immediately after this call
            // by cloth mesh component, so there is no need to transform them here.
        }
    }

    AZ::Matrix3x4 ActorClothSkinningLinear::ComputeVertexSkinningTransform(
        AZ::u32 influenceOffset, AZ::u32 numberOfInfluencesPerVertex)
    {
        AZ::Matrix3x4 vertexSkinningTransform = s_zeroMatrix3x4;
        for (size_t influenceIndex = 0; influenceIndex < numberOfInfluencesPerVertex; ++influenceIndex)
        {
            const size_t vertexInfluenceIndex = influenceOffset + influenceIndex;

            const AZ::u16 jointIndex = m_skinningInfluences[vertexInfluenceIndex].m_jointIndex;
            const float jointWeight = m_skinningInfluences[vertexInfluenceIndex].m_jointWeight;

            // Blending matrices the same way done in GPU shaders, by adding each weighted matrix element by element.
            // This operation results in a non orthogonal matrix, but it's done this way because it's fast to perform.
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
        void ApplySkinningOnNonSimulatedVertices(
            const MeshClothInfo& originalData,
            ClothComponentMesh::RenderData& renderData) override;

    private:
        MCore::DualQuaternion ComputeVertexSkinningTransform(
            AZ::u32 influenceOffset, AZ::u32 numberOfInfluencesPerVertex);

        AZStd::unordered_map<AZ::u16, MCore::DualQuaternion> m_skinningDualQuaternions;

        inline static const MCore::DualQuaternion s_zeroDualQuaternion = MCore::DualQuaternion(AZ::Quaternion::CreateZero(), AZ::Quaternion::CreateZero());
    };

    void ActorClothSkinningDualQuaternion::UpdateSkinning()
    {
        AZ_PROFILE_FUNCTION(Cloth);

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

        AZ_PROFILE_FUNCTION(Cloth);

        const size_t vertexCount = m_simulatedVertices.size();
        for (size_t index = 0; index < vertexCount; ++index)
        {
            const SimulatedVertex& vertex = m_simulatedVertices[index];
            const MCore::DualQuaternion vertexSkinningTransform =
                ComputeVertexSkinningTransform(vertex.m_influenceOffset, vertex.m_influenceCount);

            const AZ::Vector3 skinnedPosition = vertexSkinningTransform.TransformPoint(originalPositions[index].GetAsVector3());
            positions[index].Set(skinnedPosition, positions[index].GetW()); // Avoid overwriting the w component
        }
    }

    void ActorClothSkinningDualQuaternion::ApplySkinningOnNonSimulatedVertices(
        const MeshClothInfo& originalData,
        ClothComponentMesh::RenderData& renderData)
    {
        if (m_skinningDualQuaternions.empty() ||
            originalData.m_particles.empty() ||
            originalData.m_particles.size() != renderData.m_particles.size())
        {
            return;
        }

        AZ_PROFILE_FUNCTION(Cloth);

        for (const NonSimulatedVertex& vertex : m_nonSimulatedVertices)
        {
            const AZ::u32 index = vertex.m_originalVertexIndex;
            const MCore::DualQuaternion vertexSkinningTransform =
                ComputeVertexSkinningTransform(vertex.m_influenceOffset, vertex.m_influenceCount);

            const AZ::Vector3 skinnedPosition = vertexSkinningTransform.TransformPoint(originalData.m_particles[index].GetAsVector3());
            renderData.m_particles[index].Set(skinnedPosition, renderData.m_particles[index].GetW()); // Avoid overwriting the w component

            // ComputeVertexSkinningTransform is normalizing the blended dual quaternion. This means the dual
            // quaternion will not have any scale and there is no need to compute the reciprocal scale version
            // for transforming normals.
            // Note: The GPU skinning shader does the same operation.
            renderData.m_normals[index] = vertexSkinningTransform.TransformVector(originalData.m_normals[index]).GetNormalized();

            // Tangents and Bitangents are recalculated immediately after this call
            // by cloth mesh component, so there is no need to transform them here.
        }
    }

    MCore::DualQuaternion ActorClothSkinningDualQuaternion::ComputeVertexSkinningTransform(AZ::u32 influenceOffset, AZ::u32 numberOfInfluencesPerVertex)
    {
        MCore::DualQuaternion vertexSkinningTransform = s_zeroDualQuaternion;
        for (size_t influenceIndex = 0; influenceIndex < numberOfInfluencesPerVertex; ++influenceIndex)
        {
            const size_t vertexInfluenceIndex = influenceOffset + influenceIndex;

            const AZ::u16 jointIndex = m_skinningInfluences[vertexInfluenceIndex].m_jointIndex;
            const float jointWeight = m_skinningInfluences[vertexInfluenceIndex].m_jointWeight;

            const MCore::DualQuaternion& skinningDualQuaternion = m_skinningDualQuaternions.at(jointIndex);

            float flip = AZ::GetSign(vertexSkinningTransform.m_real.Dot(skinningDualQuaternion.m_real));
            vertexSkinningTransform += skinningDualQuaternion * jointWeight * flip;
        }
        // Normalizing the dual quaternion as the GPU shaders do. This will remove the scale from the transform.
        vertexSkinningTransform.Normalize();
        return vertexSkinningTransform;
    }

    AZStd::unique_ptr<ActorClothSkinning> ActorClothSkinning::Create(
        AZ::EntityId entityId,
        const MeshNodeInfo& meshNodeInfo,
        const AZStd::vector<SimParticleFormat>& originalMeshParticles,
        const size_t numSimulatedVertices,
        const AZStd::vector<int>& meshRemappedVertices)
    {
        const size_t numVertices = originalMeshParticles.size();

        AZStd::vector<SkinningInfluence> skinningInfluences;
        AZStd::vector<AZ::u32> numInfluencesPerSubmesh;
        if (!Internal::ObtainSkinningInfluences(entityId, meshNodeInfo, skinningInfluences, numInfluencesPerSubmesh))
        {
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
        for (const auto& skinningInfluence : skinningInfluences)
        {
            jointIndices.insert(skinningInfluence.m_jointIndex);
        }
        actorClothSkinning->m_jointIndices.assign(jointIndices.begin(), jointIndices.end());

        // Collect the indices for simulated and non-simulated vertices
        actorClothSkinning->m_simulatedVertices.resize(numSimulatedVertices);
        actorClothSkinning->m_nonSimulatedVertices.reserve(numVertices);


        // For each submesh...
        int meshIndexOffset = 0;
        AZ::u32 meshInfluenceOffset = 0;
        for (size_t subMeshIndex = 0; subMeshIndex < meshNodeInfo.m_subMeshes.size(); ++subMeshIndex)
        {
            const auto& subMeshInfo = meshNodeInfo.m_subMeshes[subMeshIndex];

            for (int vertexIndex = meshIndexOffset; vertexIndex < meshIndexOffset + subMeshInfo.m_numVertices; ++vertexIndex)
            {
                const AZ::u32 influenceOffset = meshInfluenceOffset + (vertexIndex - meshIndexOffset) * numInfluencesPerSubmesh[subMeshIndex];

                // The cloth cooker has previously simplified the original mesh and re-mapped the vertices
                const int remappedIndex = meshRemappedVertices[vertexIndex];

                // If the vertex has been remapped, it's part of the simulation
                if (remappedIndex >= 0)
                {
                    SimulatedVertex vertex;
                    vertex.m_influenceOffset = influenceOffset;
                    vertex.m_influenceCount = numInfluencesPerSubmesh[subMeshIndex];
                    actorClothSkinning->m_simulatedVertices[remappedIndex] = vertex;
                }

                if (remappedIndex < 0 || originalMeshParticles[vertexIndex].GetW() == 0.0f)
                {
                    NonSimulatedVertex vertex;
                    vertex.m_originalVertexIndex = static_cast<AZ::u32>(vertexIndex);
                    vertex.m_influenceOffset = influenceOffset;
                    vertex.m_influenceCount = numInfluencesPerSubmesh[subMeshIndex];
                    actorClothSkinning->m_nonSimulatedVertices.emplace_back(vertex);
                }
            }

            meshIndexOffset += subMeshInfo.m_numVertices;
            meshInfluenceOffset += subMeshInfo.m_numVertices * numInfluencesPerSubmesh[subMeshIndex];
        }

        actorClothSkinning->m_nonSimulatedVertices.shrink_to_fit();

        actorClothSkinning->m_skinningInfluences = AZStd::move(skinningInfluences);

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

