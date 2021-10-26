/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <Blast/BlastActor.h>
#include <Components/BlastMeshDataComponent.h>
#include <Family/ActorRenderManager.h>

namespace Blast
{
    ActorRenderManager::ActorRenderManager(
        AZ::Render::MeshFeatureProcessorInterface* meshFeatureProcessor, BlastMeshData* meshData, AZ::EntityId entityId,
        uint32_t chunkCount, const AZ::Vector3& scale)
        : m_meshData(meshData)
        , m_meshFeatureProcessor(meshFeatureProcessor)
        , m_chunkMeshHandles(chunkCount)
        , m_chunkActors(chunkCount, nullptr)
        , m_chunkCount(chunkCount)
        , m_scale(scale)
    {
        AZ::Render::MaterialComponentRequestBus::EventResult(
            m_materialMap, entityId, &AZ::Render::MaterialComponentRequests::GetMaterialOverrides);
    }

    void ActorRenderManager::OnActorCreated(const BlastActor& actor)
    {
        AZ_PROFILE_FUNCTION(Physics);

        const AZStd::vector<uint32_t>& chunkIndices = actor.GetChunkIndices();

        for (uint32_t chunkId : chunkIndices)
        {
            m_chunkActors[chunkId] = &actor;
            m_chunkMeshHandles[chunkId] =
                m_meshFeatureProcessor->AcquireMesh(AZ::Render::MeshHandleDescriptor{ m_meshData->GetMeshAsset(chunkId) }, m_materialMap);
        }
    }

    void ActorRenderManager::OnActorDestroyed(const BlastActor& actor)
    {
        AZ_PROFILE_FUNCTION(Physics);

        const AZStd::vector<uint32_t>& chunkIndices = actor.GetChunkIndices();

        for (uint32_t chunkId : chunkIndices)
        {
            m_meshFeatureProcessor->ReleaseMesh(m_chunkMeshHandles[chunkId]);
            m_chunkActors[chunkId] = nullptr;
        }
    }

    void ActorRenderManager::SyncMeshes()
    {
        // It is more natural to have chunk entities be transform children of rigid body entity,
        // however having them separate and manually synchronizing transform is more efficient.
        AZ_PROFILE_FUNCTION(Physics);

        for (auto chunkId = 0u; chunkId < m_chunkCount; ++chunkId)
        {
            if (m_chunkActors[chunkId])
            {
                m_meshFeatureProcessor->SetTransform(m_chunkMeshHandles[chunkId], m_chunkActors[chunkId]->GetSimulatedBody()->GetTransform(), m_scale);
            }
        }
    }
} // namespace Blast
