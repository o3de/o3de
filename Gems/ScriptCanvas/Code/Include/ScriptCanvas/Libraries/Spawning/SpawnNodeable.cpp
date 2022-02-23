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
        m_completionResults = {};
        AZ::TickBus::Handler::BusDisconnect();
    }
    
    void SpawnNodeable::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        AZStd::vector<SpawnableResult> swappedCompletionResults;
        {
            AZStd::lock_guard lock(m_idBatchMutex);
            swappedCompletionResults.swap(m_completionResults);
        }

        for (const auto& spawnResult : swappedCompletionResults)
        {
            if (spawnResult.m_entityList.empty())
            {
                continue;
            }
            CallOnSpawnCompleted(spawnResult.m_spawnTicket, AZStd::move(spawnResult.m_entityList));
        }
    }

    void SpawnNodeable::RequestSpawn(
        SpawnTicketInstance spawnTicketInstance,
        AZ::EntityId parentId,
        Data::Vector3Type translation,
        Data::Vector3Type rotation,
        Data::NumberType scale)
    {
        auto preSpawnCB = [this, parentId, translation, rotation, scale](
            [[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId,
            AzFramework::SpawnableEntityContainerView view)
        {
            AZStd::lock_guard lock(m_idBatchMutex);

            AZ::Entity* rootEntity = *view.begin();
            AzFramework::TransformComponent* entityTransform = rootEntity->FindComponent<AzFramework::TransformComponent>();

            if (entityTransform)
            {
                AZ::Vector3 rotationCopy = rotation;
                AZ::Quaternion rotationQuat = AZ::Quaternion::CreateFromEulerAnglesDegrees(rotationCopy);
                
                AzFramework::TransformComponentConfiguration transformConfig;
                transformConfig.m_parentId = parentId;
                transformConfig.m_localTransform = AZ::Transform(translation, rotationQuat, static_cast<float>(scale));
                entityTransform->SetConfiguration(transformConfig);
            }
        };

        auto spawnCompleteCB = [this, spawnTicketInstance](
            [[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId,
            AzFramework::SpawnableConstEntityContainerView view)
        {
            AZStd::lock_guard lock(m_idBatchMutex);

            SpawnableResult spawnableResult;
            spawnableResult.m_spawnTicket = spawnTicketInstance;
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
    }
} // namespace ScriptCanvas::Nodeables::Spawning
#pragma optimize("", on)
