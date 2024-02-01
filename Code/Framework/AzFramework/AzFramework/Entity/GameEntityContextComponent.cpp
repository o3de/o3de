/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

#include "GameEntityContextComponent.h"

namespace AzFramework
{
    //=========================================================================
    // Reflect
    //=========================================================================
    void GameEntityContextComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GameEntityContextComponent, AZ::Component>()
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GameEntityContextComponent>(
                    "Game Entity Context", "Owns entities in the game runtime, as well as during play-in-editor")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<GameEntityContextRequestBus>("GameEntityContextRequestBus")
                ->Attribute(AZ::Script::Attributes::Module, "entity")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Event("CreateGameEntity", &GameEntityContextRequestBus::Events::CreateGameEntityForBehaviorContext)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("DestroyGameEntity", &GameEntityContextRequestBus::Events::DestroyGameEntity)
                ->Event(
                    "DestroyGameEntityAndDescendants", &GameEntityContextRequestBus::Events::DestroyGameEntityAndDescendants)
                ->Event("ActivateGameEntity", &GameEntityContextRequestBus::Events::ActivateGameEntity)
                ->Event("DeactivateGameEntity", &GameEntityContextRequestBus::Events::DeactivateGameEntity)
                    ->Attribute(AZ::ScriptCanvasAttributes::DeactivatesInputEntity, true)
                ->Event("GetEntityName", &GameEntityContextRequestBus::Events::GetEntityName)
                ;

            behaviorContext->EBus<GameEntityContextEventBus>("GameEntityContextEventBus")
                ->Attribute(AZ::Script::Attributes::Module, "entity")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Event("OnPreGameEntitiesStarted", &GameEntityContextEventBus::Events::OnPreGameEntitiesStarted)
                ->Event("OnGameEntitiesStarted", &GameEntityContextEventBus::Events::OnGameEntitiesStarted)
                ->Event("OnGameEntitiesReset", &GameEntityContextEventBus::Events::OnGameEntitiesReset)
                ;
        }
    }

    //=========================================================================
    // GameEntityContextComponent ctor
    //=========================================================================
    GameEntityContextComponent::GameEntityContextComponent()
        : EntityContext(EntityContextId::CreateRandom())
    {
    }

    //=========================================================================
    // GameEntityContextComponent dtor
    //=========================================================================
    GameEntityContextComponent::~GameEntityContextComponent()
    {
    }

    //=========================================================================
    // Init
    //=========================================================================
    void GameEntityContextComponent::Init()
    {
    }

    //=========================================================================
    // Activate
    //=========================================================================
    void GameEntityContextComponent::Activate()
    {
        m_entityOwnershipService = AZStd::make_unique<SliceGameEntityOwnershipService>(GetContextId(), GetSerializeContext());

        InitContext();

        GameEntityContextRequestBus::Handler::BusConnect();

        m_entityVisibilityBoundsUnionSystem.Connect();
    }

    //=========================================================================
    // Deactivate
    //=========================================================================
    void GameEntityContextComponent::Deactivate()
    {
        m_entityVisibilityBoundsUnionSystem.Disconnect();

        GameEntityContextRequestBus::Handler::BusDisconnect();

        DestroyContext();

        m_entityOwnershipService.reset();
    }

    //=========================================================================
    // GameEntityContextRequestBus::ResetGameContext
    //=========================================================================
    void GameEntityContextComponent::ResetGameContext()
    {
        ResetContext();
    }

    //=========================================================================
    // GameEntityContextRequestBus::CreateGameEntity
    //=========================================================================
    AZ::Entity* GameEntityContextComponent::CreateGameEntity(const char* name)
    {
        return CreateEntity(name);
    }

    //=========================================================================
    // GameEntityContextRequestBus::CreateGameEntityForBehaviorContext
    //=========================================================================
    BehaviorEntity GameEntityContextComponent::CreateGameEntityForBehaviorContext(const char* name)
    {
        if (AZ::Entity* entity = CreateGameEntity(name))
        {
            return BehaviorEntity(entity->GetId());
        }
        return BehaviorEntity();
    }

    //=========================================================================
    // GameEntityContextRequestBus::AddGameEntity
    //=========================================================================
    void GameEntityContextComponent::AddGameEntity(AZ::Entity* entity)
    {
        AddEntity(entity);
    }


    //=========================================================================
    // CreateEntity
    //=========================================================================
    AZ::Entity* GameEntityContextComponent::CreateEntity(const char* name)
    {
        auto entity = aznew AZ::Entity(name);

        // Caller will want to configure entity before it's activated.
        entity->SetRuntimeActiveByDefault(false);

        AddEntity(entity);

        return entity;
    }


    //=========================================================================
    // OnRootEntityReloaded
    //=========================================================================
    void GameEntityContextComponent::OnRootEntityReloaded()
    {
        GameEntityContextEventBus::Broadcast(&GameEntityContextEventBus::Events::OnPreGameEntitiesStarted);
    }

    //=========================================================================
    // OnContextReset
    //=========================================================================
    void GameEntityContextComponent::OnContextReset()
    {
        GameEntityContextEventBus::Broadcast(&GameEntityContextEventBus::Events::OnGameEntitiesReset);
    }

    //=========================================================================
    // GameEntityContextComponent::ValidateEntitiesAreValidForContext
    //=========================================================================
    bool GameEntityContextComponent::ValidateEntitiesAreValidForContext(const EntityList& entities)
    {
        // All entities in a prefab being instantiated in the level editor should
        // have the TransformComponent on them. Since it is not possible to create
        // a prefab with entities from different contexts, it is OK to check
        // the first entity only
        if (entities.size() > 0)
        {
            return entities[0]->FindComponent<AzFramework::TransformComponent>() != nullptr;
        }

        return true;
    }

    //=========================================================================
    // GameEntityContextComponent::OnContextEntitiesAdded
    //=========================================================================
    void GameEntityContextComponent::OnContextEntitiesAdded(const EntityList& entities)
    {
        EntityContext::OnContextEntitiesAdded(entities);

    #if (AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING)
        auto timeOfLastEventPump = AZStd::chrono::steady_clock::now();
        auto PumpSystemEventsIfNeeded = [&timeOfLastEventPump]()
        {
            static const AZStd::chrono::milliseconds maxMillisecondsBetweenSystemEventPumps(AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING_INTERVAL_MS);
            const auto now = AZStd::chrono::steady_clock::now();
            if (now - timeOfLastEventPump > maxMillisecondsBetweenSystemEventPumps)
            {
                timeOfLastEventPump = now;
                ApplicationRequests::Bus::Broadcast(&ApplicationRequests::PumpSystemEventLoopUntilEmpty);
            }
        };
    #endif // (AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING)

        for (AZ::Entity* entity : entities)
        {
            if (entity->GetState() == AZ::Entity::State::Constructed)
            {
                entity->Init();
            #if (AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING)
                PumpSystemEventsIfNeeded();
            #endif // (AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING)
            }
        }

        for (AZ::Entity* entity : entities)
        {
            if (entity->GetState() == AZ::Entity::State::Init)
            {
                if (entity->IsRuntimeActiveByDefault())
                {
                    entity->Activate();
                #if (AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING)
                    PumpSystemEventsIfNeeded();
                #endif // (AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING)
                }
            }
        }
    }

    //=========================================================================
    // GameEntityContextComponent::DestroyGameEntityById
    //=========================================================================
    void GameEntityContextComponent::DestroyGameEntity(const AZ::EntityId& id)
    {
        DestroyGameEntityInternal(id, false);
    }

    //=========================================================================
    // GameEntityContextComponent::DestroyGameEntityAndDescendantsById
    //=========================================================================
    void GameEntityContextComponent::DestroyGameEntityAndDescendants(const AZ::EntityId& id)
    {
        DestroyGameEntityInternal(id, true);
    }

    //=========================================================================
    // GameEntityContextComponent::DestroyGameEntityInternal
    //=========================================================================
    void GameEntityContextComponent::DestroyGameEntityInternal(const AZ::EntityId& entityId, bool destroyChildren)
    {
        AZStd::vector<AZ::EntityId> entityIdsToBeDeleted;

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        if (entity)
        {
            if (destroyChildren)
            {
                AZ::TransformBus::EventResult(entityIdsToBeDeleted, entityId, &AZ::TransformBus::Events::GetAllDescendants);
            }

            // Inserting the parent to the list before its children; it will be deleted last by the reverse iterator
            entityIdsToBeDeleted.insert(entityIdsToBeDeleted.begin(), entityId);
        }

        for (AZStd::vector<AZ::EntityId>::reverse_iterator entityIdIter = entityIdsToBeDeleted.rbegin();
            entityIdIter != entityIdsToBeDeleted.rend(); ++entityIdIter)
        {
            AZ::Entity* currentEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(currentEntity, &AZ::ComponentApplicationBus::Events::FindEntity, *entityIdIter);
            if (currentEntity)
            {
                if (currentEntity->GetEntitySpawnTicketId() > 0)
                {
                    SpawnableEntitiesDefinition* spawnableEntitiesInterface = SpawnableEntitiesInterface::Get();
                    AZ_Assert(spawnableEntitiesInterface != nullptr, "SpawnableEntitiesInterface is not found.");
                    spawnableEntitiesInterface->RetrieveTicket(
                        currentEntity->GetEntitySpawnTicketId(),
                        [spawnableEntitiesInterface, currentEntity](EntitySpawnTicket&& entitySpawnTicket)
                        {
                            if (entitySpawnTicket.IsValid())
                            {
                                spawnableEntitiesInterface->DespawnEntity(currentEntity->GetId(), entitySpawnTicket);
                            }
                        });
                    continue;
                }

                if (currentEntity->GetState() == AZ::Entity::State::Active)
                {
                    // Deactivate the entity, we'll destroy it as soon as it is safe.
                    currentEntity->Deactivate();
                }
                else
                {
                    // Don't activate the entity, it will be destroyed.
                    currentEntity->SetRuntimeActiveByDefault(false);
                }
            }
        }

        // Queue the entity destruction on the tick bus for safety, this guarantees that we will not attempt to destroy
        // an entity during activation.
        AZStd::function<void()> destroyEntity = [this,entityIdsToBeDeleted]() mutable
        {
            for (AZStd::vector<AZ::EntityId>::reverse_iterator entityIdIter = entityIdsToBeDeleted.rbegin();
                 entityIdIter != entityIdsToBeDeleted.rend(); ++entityIdIter)
            {
                EntityContext::DestroyEntityById(*entityIdIter);
            }
        };

        AZ::TickBus::QueueFunction(destroyEntity);
    }

    //=========================================================================
    // GameEntityContextComponent::ActivateGameEntity
    //=========================================================================
    void GameEntityContextComponent::ActivateGameEntity(const AZ::EntityId& entityId)
    {
        ActivateEntity(entityId);
    }

    //=========================================================================
    // GameEntityContextComponent::DeactivateGameEntity
    //=========================================================================
    void GameEntityContextComponent::DeactivateGameEntity(const AZ::EntityId& entityId)
    {
        DeactivateEntity(entityId);
    }

    //=========================================================================
    // EntityContextEventBus::LoadFromStream
    //=========================================================================
    bool GameEntityContextComponent::LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds)
    {
        if (m_entityOwnershipService->LoadFromStream(stream, remapIds))
        {
            GameEntityContextEventBus::Broadcast(&GameEntityContextEventBus::Events::OnGameEntitiesStarted);
            return true;
        }

        return false;
    }

    //=========================================================================
    // GameEntityContextRequestBus::GetEntityName
    //=========================================================================
    AZStd::string GameEntityContextComponent::GetEntityName(const AZ::EntityId& id)
    {
        AZStd::string entityName;
        AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, id);
        return entityName;
    }
} // namespace AzFramework
