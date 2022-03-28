/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnableBus.h>
#include <AzFramework/Spawnable/SpawnableMediator.h>
#include <AzFramework/Spawnable/SpawnableNotificationsHandler.h>

namespace AzFramework
{
    void SpawnableMediator::Reflect(AZ::ReflectContext* context)
    {
        SpawnableAssetRef::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<SpawnableMediator>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SpawnableMediator>("SpawnableMediator")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab/Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "prefabs")
                ->Method("CreateSpawnTicket", &SpawnableMediator::CreateSpawnTicket)
                ->Method("Spawn", &SpawnableMediator::Spawn)
                ->Method("Despawn", &SpawnableMediator::Despawn)
                ->Method("Clear", &SpawnableMediator::Clear);

            behaviorContext->EBus<SpawnableNotificationsBus>("SpawnableNotificationsBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab/Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "prefabs")
                ->Handler<SpawnableNotificationsHandler>();
        }
    }

    SpawnableMediator::SpawnableMediator()
        : m_interface(SpawnableEntitiesInterface::Get())
    {
    }

    SpawnableMediator::~SpawnableMediator()
    {
        Clear();
    }

    void SpawnableMediator::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        ProcessDespawnedResults();
        ProcessSpawnedResults();
        AZ::TickBus::Handler::BusDisconnect();
    }

    EntitySpawnTicket SpawnableMediator::CreateSpawnTicket(const SpawnableAssetRef& spawnableAsset)
    {
        return EntitySpawnTicket(spawnableAsset.m_asset);
    }

    bool SpawnableMediator::Spawn(
        EntitySpawnTicket spawnTicket,
        AZ::EntityId parentId,
        AZ::Vector3 translation,
        AZ::Vector3 rotation,
        float scale)
    {
        if (!spawnTicket.IsValid())
        {
            AZ_Error("Spawnables", false, "EntitySpawnTicket used for spawning is invalid.");
            return false;
        }

        auto preSpawnCB = [this, parentId, translation, rotation, scale](
                              [[maybe_unused]] EntitySpawnTicket::Id ticketId,
            SpawnableEntityContainerView view)
        {
            AZStd::lock_guard lock(m_mutex);

            AZ::Entity* rootEntity = *view.begin();
            TransformComponent* entityTransform = rootEntity->FindComponent<TransformComponent>();

            if (entityTransform)
            {
                AZ::Vector3 rotationCopy = rotation;
                AZ::Quaternion rotationQuat = AZ::Quaternion::CreateFromEulerAnglesDegrees(rotationCopy);

                TransformComponentConfiguration transformConfig;
                transformConfig.m_parentId = parentId;
                transformConfig.m_localTransform = AZ::Transform(translation, rotationQuat, scale);
                entityTransform->SetConfiguration(transformConfig);
            }
        };

        auto spawnCompleteCB =
            [this,
             spawnTicket]([[maybe_unused]] EntitySpawnTicket::Id ticketId, SpawnableConstEntityContainerView view)
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
            m_spawnedResults.push_back(spawnableResult);
        };

        SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_preInsertionCallback = AZStd::move(preSpawnCB);
        optionalArgs.m_completionCallback = AZStd::move(spawnCompleteCB);
        SpawnableEntitiesInterface::Get()->SpawnAllEntities(spawnTicket, AZStd::move(optionalArgs));

        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }

        return true;
    }

    bool SpawnableMediator::Despawn(EntitySpawnTicket spawnTicket)
    {
        if (!spawnTicket.IsValid())
        {
            AZ_Error("Spawnables", false, "EntitySpawnTicket used for despawning is invalid.");
            return false;
        }

        auto despawnCompleteCB = [this, spawnTicket]([[maybe_unused]] EntitySpawnTicket::Id ticketId)
        {
            AZStd::lock_guard lock(m_mutex);
            // SpawnTicket instance is cached instead of SpawnTicketId to simplify managing its lifecycle on Script Canvas
            // and to provide easier access to it in OnDespawn callback
            m_despawnedResults.push_back(spawnTicket);
        };

        DespawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(despawnCompleteCB);
        SpawnableEntitiesInterface::Get()->DespawnAllEntities(spawnTicket, AZStd::move(optionalArgs));

        
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }

        return true;
    }

    void SpawnableMediator::Clear()
    {
        m_spawnedResults.clear();
        m_spawnedResults.shrink_to_fit();
        m_despawnedResults.clear();
        m_spawnedResults.shrink_to_fit();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SpawnableMediator::ProcessSpawnedResults()
    {
        AZStd::vector<SpawnableResult> swappedSpawnedResults;
        {
            AZStd::lock_guard lock(m_mutex);
            swappedSpawnedResults.swap(m_spawnedResults);
        }

        for (const auto& spawnResult : swappedSpawnedResults)
        {
            if (spawnResult.m_entityList.empty())
            {
                continue;
            }
            SpawnableNotificationsBus::Event(
                spawnResult.m_spawnTicket.GetId(), &SpawnableNotifications::OnSpawn, spawnResult.m_spawnTicket,
                move(spawnResult.m_entityList));
        }
    }

    void SpawnableMediator::ProcessDespawnedResults()
    {
        AZStd::vector<EntitySpawnTicket> swappedDespawnedResults;
        {
            AZStd::lock_guard lock(m_mutex);
            swappedDespawnedResults.swap(m_despawnedResults);
        }

        for (const auto& spawnTicket : swappedDespawnedResults)
        {
            SpawnableNotificationsBus::Event(spawnTicket.GetId(), &SpawnableNotifications::OnDespawn, spawnTicket);
        }
    }
} // namespace AzFramework
