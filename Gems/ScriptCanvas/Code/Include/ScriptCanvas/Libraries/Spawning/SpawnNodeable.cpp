/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>

#pragma optimize("", off)
namespace ScriptCanvas::Nodeables::Spawning
{
    SpawnNodeable::SpawnNodeable([[maybe_unused]] const SpawnNodeable& rhs)
    {}

    SpawnNodeable& SpawnNodeable::operator=([[maybe_unused]] SpawnNodeable& rhs)
    {
        return *this;
    }

    void SpawnNodeable::OnInitializeExecutionState()
    {
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SpawnNodeable::OnDeactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }
    
    void SpawnNodeable::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        AZStd::vector<SpawnableResult> swappedPreInsertionResults;
        AZStd::vector<SpawnableResult> swappedCompletionResults;
        {
            AZStd::lock_guard lock(m_idBatchMutex);
            swappedPreInsertionResults.swap(m_preInsertionResults);
            swappedCompletionResults.swap(m_completionResults);
        }

        for (const auto& spawnResult : swappedPreInsertionResults)
        {
            if (spawnResult.m_entityList.empty())
            {
                continue;
            }
            CallOnInsertion(spawnResult.m_spawnTicketId, AZStd::move(spawnResult.m_entityList));
        }

        for (const auto& spawnResult : swappedCompletionResults)
        {
            if (spawnResult.m_entityList.empty())
            {
                continue;
            }
            CallOnCompletion(spawnResult.m_spawnTicketId, AZStd::move(spawnResult.m_entityList));
        }
    }

    SpawnTicketInstance SpawnNodeable::RequestSpawn(SpawnTicketInstance spawnTicketInstance)
    {
        auto preSpawnCB = [this](AzFramework::EntitySpawnTicket::Id ticketId,
            AzFramework::SpawnableEntityContainerView view)
        {
            AZStd::lock_guard lock(m_idBatchMutex);

            SpawnableResult spawnableResult;
            spawnableResult.m_spawnTicketId = ticketId;
            spawnableResult.m_entityList.reserve(view.size());
            for (const AZ::Entity* entity : view)
            {
                spawnableResult.m_entityList.emplace_back(entity->GetId());
            }
            m_preInsertionResults.push_back(spawnableResult);

            AZ::Entity* rootEntity = *view.begin();

            AzFramework::TransformComponent* entityTransform = rootEntity->FindComponent<AzFramework::TransformComponent>();

            if (entityTransform)
            {
                AZ::Vector3 rotationCopy = AZ::Vector3::CreateZero();
                AZ::Quaternion rotationQuat = AZ::Quaternion::CreateFromEulerAnglesDegrees(rotationCopy);

                entityTransform->SetWorldTM(AZ::Transform(AZ::Vector3::CreateZero(), rotationQuat, 1.0f));
            }
        };

        auto spawnCompleteCB = [this](AzFramework::EntitySpawnTicket::Id ticketId,
            AzFramework::SpawnableConstEntityContainerView view)
        {
            AZStd::lock_guard lock(m_idBatchMutex);

            SpawnableResult spawnableResult;
            spawnableResult.m_spawnTicketId = ticketId;
            spawnableResult.m_entityList.reserve(view.size());
            for (const AZ::Entity* entity : view)
            {
                spawnableResult.m_entityList.emplace_back(entity->GetId());
            }
            m_completionResults.push_back(spawnableResult);
        };

        AzFramework::SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_preInsertionCallback = AZStd::move(preSpawnCB);
        optionalArgs.m_completionCallback = AZStd::move(spawnCompleteCB);
        AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(*spawnTicketInstance.m_ticket, AZStd::move(optionalArgs));
        return spawnTicketInstance;
    }
} // namespace ScriptCanvas::Nodeables::Spawning
#pragma optimize("", on)
