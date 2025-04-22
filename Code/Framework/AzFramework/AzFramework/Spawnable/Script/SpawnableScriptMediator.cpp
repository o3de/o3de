/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptBus.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptMediator.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptNotificationsHandler.h>

namespace AzFramework::Scripts
{
    void SpawnableScriptMediator::Reflect(AZ::ReflectContext* context)
    {
        SpawnableScriptAssetRef::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext
                ->Class<SpawnableScriptMediator>()
                ->Version(0)
            ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SpawnableScriptMediator>("SpawnableScriptMediator")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab/Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "prefabs")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("CreateSpawnTicket", &SpawnableScriptMediator::CreateSpawnTicket)
                ->Method("Spawn", &SpawnableScriptMediator::Spawn)
                ->Method("SpawnAndParent", &SpawnableScriptMediator::SpawnAndParent)
                ->Method("SpawnAndParentAndTransform", &SpawnableScriptMediator::SpawnAndParentAndTransform)
                ->Method("Despawn", &SpawnableScriptMediator::Despawn)
                ->Method("Clear", &SpawnableScriptMediator::Clear);

            behaviorContext->EBus<SpawnableScriptNotificationsBus>("SpawnableScriptNotificationsBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab/Spawning")
                ->Attribute(AZ::Script::Attributes::Module, "prefabs")
                ->Handler<SpawnableScriptNotificationsHandler>();
        }
    }

    SpawnableScriptMediator::SpawnableScriptMediator()
        : m_sentinel(AZStd::make_shared<CallbackSentinel>())
    {
    }

    SpawnableScriptMediator::~SpawnableScriptMediator()
    {
        Clear();
    }

    void SpawnableScriptMediator::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        ProcessResults();
        AZ::TickBus::Handler::BusDisconnect();
    }

    EntitySpawnTicket SpawnableScriptMediator::CreateSpawnTicket(const SpawnableScriptAssetRef& spawnableAsset)
    {
        EntitySpawnTicket ticket = EntitySpawnTicket(spawnableAsset.GetAsset());
        m_cachedSpawnTickets.insert(ticket);
        return ticket;
    }

    bool SpawnableScriptMediator::Spawn(EntitySpawnTicket spawnTicket)
    {
        static AZ::EntityId parentId;
        return SpawnAndParentAndTransform(
            spawnTicket,
            parentId,
            AZ::Vector3::CreateZero(), // translation
            AZ::Vector3::CreateZero(), // rotation
            1.0f); // scale
    }

    bool SpawnableScriptMediator::SpawnAndParent(EntitySpawnTicket spawnTicket, const AZ::EntityId& parentId)
    {
        return SpawnAndParentAndTransform(
            spawnTicket,
            parentId,
            AZ::Vector3::CreateZero(), // translation
            AZ::Vector3::CreateZero(), // rotation
            1.0f); // scale
    }

    bool SpawnableScriptMediator::SpawnAndParentAndTransform(
        EntitySpawnTicket spawnTicket,
        const AZ::EntityId& parentId,
        const AZ::Vector3& translation,
        const AZ::Vector3& rotation,
        float scale)
    {
        if (!spawnTicket.IsValid())
        {
            AZ_Error("Spawnables", false, "EntitySpawnTicket used for spawning is invalid.");
            return false;
        }

        AZStd::weak_ptr<CallbackSentinel> weakPtr = m_sentinel;
        auto preSpawnCB = [weakPtr, parentId, translation, rotation, scale]
            ([[maybe_unused]] EntitySpawnTicket::Id ticketId, SpawnableEntityContainerView view)
        {
            if (!weakPtr.lock())
            {
                return;
            }
            AZ::Entity* containerEntity = *view.begin();
            TransformComponent* entityTransform = containerEntity->FindComponent<TransformComponent>();

            if (entityTransform)
            {
                AZ::Quaternion rotationQuat = AZ::Quaternion::CreateFromEulerAnglesDegrees(rotation);

                TransformComponentConfiguration transformConfig;
                transformConfig.m_parentId = parentId;
                transformConfig.m_localTransform = AZ::Transform(translation, rotationQuat, scale);
                entityTransform->SetConfiguration(transformConfig);
            }
        };

        auto spawnCompleteCB =
            [this, weakPtr, spawnTicket]
            ([[maybe_unused]] EntitySpawnTicket::Id ticketId, SpawnableConstEntityContainerView view)
        {
            if (!weakPtr.lock())
            {
                return;
            }
            SpawnResult spawnResult;
            // SpawnTicket instance is cached instead of SpawnTicketId to simplify managing its lifecycle on Script Canvas
            // and to provide easier access to it in OnSpawnCompleted callback
            spawnResult.m_spawnTicket = spawnTicket;
            spawnResult.m_entityList.reserve(view.size());
            for (const AZ::Entity* entity : view)
            {
                spawnResult.m_entityList.emplace_back(entity->GetId());
            }
            QueueProcessResult(spawnResult);
        };

        SpawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_preInsertionCallback = AZStd::move(preSpawnCB);
        optionalArgs.m_completionCallback = AZStd::move(spawnCompleteCB);
        SpawnableEntitiesInterface::Get()->SpawnAllEntities(spawnTicket, AZStd::move(optionalArgs));

        return true;
    }

    bool SpawnableScriptMediator::Despawn(EntitySpawnTicket spawnTicket)
    {
        if (!spawnTicket.IsValid())
        {
            AZ_Error("Spawnables", false, "EntitySpawnTicket used for despawning is invalid.");
            return false;
        }

        AZStd::weak_ptr<CallbackSentinel> weakPtr = m_sentinel;
        auto despawnCompleteCB = [this, weakPtr, spawnTicket]
            ([[maybe_unused]] EntitySpawnTicket::Id ticketId)
        {
            if (!weakPtr.lock())
            {
                return;
            }
            // SpawnTicket instance is cached instead of SpawnTicketId to simplify managing its lifecycle on Script Canvas
            // and to provide easier access to it in OnDespawn callback
            DespawnResult despawnResult;
            despawnResult.m_spawnTicket = spawnTicket;
            QueueProcessResult(despawnResult);
        };

        DespawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(despawnCompleteCB);
        SpawnableEntitiesInterface::Get()->DespawnAllEntities(spawnTicket, AZStd::move(optionalArgs));

        return true;
    }

    void SpawnableScriptMediator::Clear()
    {
        m_cachedSpawnTickets.clear();
        m_resultCommands.clear();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SpawnableScriptMediator::QueueProcessResult(const ResultCommand& resultCommand)
    {
        AZStd::lock_guard lock(m_mutex);
        m_resultCommands.push_back(resultCommand);
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void SpawnableScriptMediator::ProcessResults()
    {
        AZStd::vector<ResultCommand> swappedResultCommands;
        {
            AZStd::lock_guard lock(m_mutex);
            swappedResultCommands.swap(m_resultCommands);
        }

        for (ResultCommand& command : swappedResultCommands)
        {
            auto visitor = [](auto&& command)
            {
                using CommandType = AZStd::decay_t<decltype(command)>;
                if constexpr (AZStd::is_same_v<CommandType, SpawnResult>)
                {
                    if (!command.m_entityList.empty())
                    {
                        SpawnableScriptNotificationsBus::Event(
                            command.m_spawnTicket.GetId(), &SpawnableScriptNotifications::OnSpawn, command.m_spawnTicket,
                            move(command.m_entityList));
                    }
                }
                if constexpr (AZStd::is_same_v<CommandType, DespawnResult>)
                {
                    SpawnableScriptNotificationsBus::Event(
                        command.m_spawnTicket.GetId(), &SpawnableScriptNotifications::OnDespawn, command.m_spawnTicket);
                }
            };
            AZStd::visit(visitor, command);
        }
    }
} // namespace AzFramework::Scripts
