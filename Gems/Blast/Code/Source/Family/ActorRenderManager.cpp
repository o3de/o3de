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

#include "StdAfx.h"

#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

        const AZStd::vector<uint32_t>& chunkIndices = actor.GetChunkIndices();

        for (uint32_t chunkId : chunkIndices)
        {
            m_chunkActors[chunkId] = &actor;
            m_chunkMeshHandles[chunkId] =
                m_meshFeatureProcessor->AcquireMesh(m_meshData->GetMeshAsset(chunkId), m_materialMap);
        }
    }

    void ActorRenderManager::OnActorDestroyed(const BlastActor& actor)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

        for (auto chunkId = 0u; chunkId < m_chunkCount; ++chunkId)
        {
            if (m_chunkActors[chunkId])
            {
                auto transform = m_chunkActors[chunkId]->GetWorldBody()->GetTransform();
                // Multiply by scale because the transform on the world body does not store scale
                transform.MultiplyByScale(m_scale);
                m_meshFeatureProcessor->SetTransform(m_chunkMeshHandles[chunkId], transform);
            }
        }
    }
} // namespace Blast
