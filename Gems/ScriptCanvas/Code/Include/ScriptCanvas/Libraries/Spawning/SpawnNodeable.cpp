/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>
#include <AzFramework/Components/TransformComponent.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    SpawnNodeable::SpawnNodeable([[maybe_unused]] const SpawnNodeable& rhs)
    {
        // this method is required by Script Canvas, left intentionally blank to avoid copying m_completionResults
    }

    SpawnNodeable& SpawnNodeable::operator=([[maybe_unused]] const SpawnNodeable& rhs)
    {
        // this method is required by Script Canvas, left intentionally blank to avoid copying m_completionResults
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
        m_completionResults.clear(); // clears any cached SpawnTickets that may remain so everything despawns
        m_completionResults.shrink_to_fit();
    }

    void SpawnNodeable::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        AZStd::vector<SpawnableResult> swappedCompletionResults;
        {
            AZStd::lock_guard lock(m_mutex);
            swappedCompletionResults.swap(m_completionResults);
        }

        for (const auto& spawnResult : swappedCompletionResults)
        {
            if (spawnResult.m_entityList.empty())
            {
                continue;
            }
            CallOnSpawnCompleted(spawnResult.m_spawnTicket, move(spawnResult.m_entityList));
        }
    }

    void SpawnNodeable::RequestSpawn(
        SpawnTicketInstance spawnTicket,
        AZ::EntityId parentId,
        Data::Vector3Type translation,
        Data::Vector3Type rotation,
        Data::NumberType scale)
    {
        auto preSpawnCB = [this, parentId, translation, rotation, scale](
            [[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId,
            AzFramework::SpawnableEntityContainerView view)
        {
            AZStd::lock_guard lock(m_mutex);

            AZ::Entity* rootEntity = *view.begin();
            AzFramework::TransformComponent* entityTransform = rootEntity->FindComponent<AzFramework::TransformComponent>();

            if (entityTransform)
            {
                AZ::Vector3 rotationCopy = rotation;
                AZ::Quaternion rotationQuat = AZ::Quaternion::CreateFromEulerAnglesDegrees(rotationCopy);

                AzFramework::TransformComponentConfiguration transformConfig;
                transformConfig.m_parentId = parentId;
                transformConfig.m_localTransform = AZ::Transform(translation, rotationQuat, aznumeric_cast<float>(scale));
                entityTransform->SetConfiguration(transformConfig);
            }
        };

        auto spawnCompleteCB = [this, spawnTicket](
            [[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId,
            AzFramework::SpawnableConstEntityContainerView view)
        {
            AZStd::lock_guard lock(m_mutex);

            SpawnableResult spawnableResult;
            // SpawnTicket instance is cached instead of SpawnTicketId to simplify managing its lifecycle on Script Canvas
            // and to provide easier access to it in OnSpawnCompleted callback
            spawnableResult.m_spawnTicket = spawnTicket;
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
        AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(*spawnTicket.m_ticket, AZStd::move(optionalArgs));
    }
} // namespace ScriptCanvas::Nodeables::Spawning
