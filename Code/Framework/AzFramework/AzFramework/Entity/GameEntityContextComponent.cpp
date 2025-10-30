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
#include <AzCore/Component/EntityActiveSystemBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

#include "GameEntityContextComponent.h"

namespace AzFramework
{
    //=========================================================================
    // Hierarchy Handling Utilities
    //  - EraseValue
    //  - EnsureChildList
    //  - FindEntity
    //=========================================================================
    template<class T>
    static bool EraseValue(AZStd::vector<T>& vec, const T& value)
    {
        // 1) "Remove": move all elements != value to the front.
        auto it = AZStd::remove(vec.begin(), vec.end(), value);

        // After this:
        // - [vec.begin(), it)   contains the elements you KEEP, compacted
        // - [it, vec.end())     contains now-meaningless "garbage" (old values)

        // 2) If anything was actually "removed", trim the vector's size.
        if (it != vec.end())
        {
            vec.erase(it, vec.end()); // physically shorten the vector
            return true;              // we did remove at least one element
        }
        return false;                 // nothing matched 'value'
    }

    static AZStd::vector<AZ::EntityId>& EnsureChildList(AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::EntityId>, AZStd::hash<AZ::EntityId>>& map, const AZ::EntityId& key)
    {
        // Finding Parent ID in parentChildTree.
        auto it = map.find(key);
        if (it == map.end())
        {
            //If it doesn't exist, add it with a blank vector as its value pair.
            it = map.emplace(key, AZStd::vector<AZ::EntityId>{}).first;
        }
        //One way or another, return the children vector.
        return it->second;
    }

    AZ::Entity* FindEntity(AZ::EntityId id)
    {
        AZ::Entity* e = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            e, &AZ::ComponentApplicationBus::Events::FindEntity, id);
        return e;
    }

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
                ->Event("ActivateGameEntityAndDescendants", &GameEntityContextRequestBus::Events::ActivateGameEntityAndDescendants)
                ->Event("DeactivateGameEntity", &GameEntityContextRequestBus::Events::DeactivateGameEntity)
                    ->Attribute(AZ::ScriptCanvasAttributes::DeactivatesInputEntity, true)
                ->Event("DeactivateGameEntityAndDescendants", &GameEntityContextRequestBus::Events::DeactivateGameEntityAndDescendants)
                    ->Attribute(AZ::ScriptCanvasAttributes::DeactivatesInputEntity, true)
                ->Event("GetEntityName", &GameEntityContextRequestBus::Events::GetEntityName)
                ->Event("SetGameEntityParent", &GameEntityContextRequestBus::Events::SetGameEntityParent)
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
        AddEntityToParentChildTree(entity);
        AddEntity(entity);
    }


    //=========================================================================
    // CreateEntity
    //=========================================================================
    AZ::Entity* GameEntityContextComponent::CreateEntity(const char* name)
    {
        auto entity = aznew AZ::Entity(name);

        // Caller will want to configure entity before it's activated.
        entity->SetStartActive(false);

        AddGameEntity(entity);

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

        // Assure the Parent index is aquired by now.
        if(parentActiveTypeIndex == std::numeric_limits<size_t>::max())
        {
            AZ::EntityActiveSystemRequestBus::BroadcastResult(parentActiveTypeIndex, &AZ::EntityActiveSystemRequests::RegisterEntityActiveType, PARENT_ACTIVE_TYPE_NAME);
        }

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

        // Preprocess active state hierarchy.
        // Parenting happened in Init(), this should mean Tree is updated with proper parent -> child relationships.
        // Assuming entity order is top to bottom. We should capture the state from one to the next.
        for (AZ::Entity* entity : entities)
        {
            if (!entity) continue;
            AZ::EntityId id = entity->GetId();

            //Get parent if valid.
            AZ::EntityId parent = AZ::EntityId();
            if (auto it = parentOf.find(id); it != parentOf.end())
            { 
                parent = it->second; 
            }

            bool pEff = true;
            if(parent.IsValid())
            {
                AZ::Entity* pEnt = FindEntity(parent);
                if (pEnt) { pEff = pEnt->IsEffectivelyActive(); }
            }

            entity->SetActiveByTypeIndex(parentActiveTypeIndex, pEff, false);
        }

        for (AZ::Entity* entity : entities)
        {
            if (entity->GetState() == AZ::Entity::State::Init)
            {
                // Check for active state. if so, process Activation.
                if (entity->IsEffectivelyActive())
                {
                    entity->EvaluateEffectiveActiveState();
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
                RemoveEntityFromParentChildTree(entity);

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
                    currentEntity->SetStartActive(false);
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
        // Verify that this context has the right to perform operations on the entity
        bool validEntity = IsOwnedByThisContext(entityId);
        AZ_Warning("GameEntityContext", validEntity, "Entity with id %llu does not belong to the game context.", entityId);

        if (validEntity)
        {
            // Look up the entity and activate it.
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
            if (entity)
            {
                // Safety Check: Is the entity initialized?
                if (entity->GetState() == AZ::Entity::State::Constructed)
                {
                    AZ_Warning("GameEntityContext", false, "Entity with id %llu was not initialized before activation requested.", entityId);
                    entity->Init();
                }

                if (entity->GetState() == AZ::Entity::State::Init)
                {
                    entity->SetActive(true);
                }
            }
        }
    }

    //=========================================================================
    // GameEntityContextComponent::ActivateGameEntityAndDesendants
    //=========================================================================
    void GameEntityContextComponent::ActivateGameEntityAndDescendants(AZ::EntityId rootEntityId, bool updateRoot)
    {
        // Verify that this context has the right to perform operations on the entity
        if (!IsOwnedByThisContext(rootEntityId))
        {
            AZ_Warning("EntityContext", false, "Entity %llu not owned by this context.", rootEntityId);
            return;
        }

        AZ::Entity* rootEntity = FindEntity(rootEntityId);
        if (!rootEntity)
        {
            AZ_Warning("EntityContext", false, "Root entity %llu not found.", rootEntityId);
            return;
        }

        AZStd::vector<AZ::EntityId> tree;
        tree.reserve(64);
        GetEntityTreeFromRootEntity(rootEntityId, tree);

        if(tree.empty()) 
        {
            AZ_Printf("EntityContext", "Activate and descendants: tree is empty, cancelling out.");
            return; 
        }

        // Loop through the vector and activate top to bottom.
        for (size_t i = 0; i < tree.size(); ++i)
        {
            const AZ::EntityId id = tree[i];
            AZ::Entity* e = FindEntity(tree[i]);
            if (!e) { continue; }

            bool changed = false;
            if (id == rootEntityId)
            {
                // Either we set root true because we want it true, or because other systems have evaluated, we set the root's aprent state to true.
                // This enables reparenting + activation ordering that's out of order.
                if(updateRoot)
                {
                    changed = e->SetActive(true);
                }
                else
                {
                    changed = e->SetActiveByTypeIndex(parentActiveTypeIndex, true);
                }
                
                // One way or another, if the root doesn't change there's no reason to change the children.
                if(!changed)
                {
                    AZ_Printf("EntityContext", "Activate and descendants: Root didn't change. Cancelling out.");
                    return; 
                }
            }
            else
            {
                // Now that we evaluated the root, and it changed, we can start stepping through children to say the root did infact change to true.
                changed = e->SetActiveByTypeIndex(parentActiveTypeIndex, true);
            }

            if(!changed)
            {   
                AZ_Printf("EntityContext", "Activate and descendants: %s didn't change state, adding to ignore.", e->GetName().c_str());
                // If, despite changing the parent state to true, this did not change the child. That means it's local state is inactive.
                // If that's the case, it's already processed it's children.
                PruneDescendantsFromTreeInPlace(id, tree, i);
            }
        }
    }

    //=========================================================================
    // GameEntityContextComponent::DeactivateGameEntity
    //=========================================================================
    void GameEntityContextComponent::DeactivateGameEntity(const AZ::EntityId& entityId)
    {
        // Verify that this context has the right to perform operations on the entity
        bool validEntity = IsOwnedByThisContext(entityId);
        AZ_Warning("GameEntityContext", validEntity, "Entity with id %llu does not belong to the game context.", entityId);

        if (validEntity)
        {
            // Then look up the entity and deactivate it.
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
            if (entity)
            {
                switch (entity->GetState())
                {
                case AZ::Entity::State::Activating:
                    // Queue deactivate to trigger next frame
                    AZ::TickBus::QueueFunction(&AZ::Entity::SetActive, entity, false, true);
                    break;

                case AZ::Entity::State::Active:
                    // Deactivate immediately
                    entity->SetActive(false);
                    break;

                default:
                    // Don't do anything, it's not even active.
                    break;
                }
            }
        }
    }

    //=========================================================================
    // GameEntityContextComponent::DeactivateEntityAndDesendants
    //=========================================================================
    void GameEntityContextComponent::DeactivateGameEntityAndDescendants(AZ::EntityId rootEntityId, bool updateRoot)
    {
        // Verify that this context has the right to perform operations on the entity
        if (!IsOwnedByThisContext(rootEntityId))
        {
            AZ_Warning("EntityContext", false, "Entity %llu not owned by this context.", rootEntityId);
            return;
        }
        
        AZ::Entity* rootEntity = FindEntity(rootEntityId);
        if (!rootEntity)
        {
            AZ_Warning("EntityContext", false, "Root entity %llu not found.", rootEntityId);
            return;
        }

        AZStd::vector<AZ::EntityId> tree;
        tree.reserve(64);
        GetEntityTreeFromRootEntity(rootEntityId, tree);

        if(tree.empty()) { return; }
        
        // Loop through the vector and disable bottom to top.
        for (size_t i = tree.size(); i-- > 0; )
        {
            AZ::EntityId entityId = tree[i];
            if(entityId == rootEntityId) 
            {
                if(updateRoot) { rootEntity -> SetActive(false); }
                else { rootEntity->SetActiveByTypeIndex(parentActiveTypeIndex, false); }  
            }
            else
            {
                AZ::Entity* e = FindEntity(entityId);
                
                if(!e) { continue; }

                // Because we're going bottom to top, there's no need to check if the state has changed.
                e->SetActiveByTypeIndex(parentActiveTypeIndex, false);
            }
        }
    }

    //=========================================================================
    // EntityContextEventBus::SetGameEntityParent
    //=========================================================================
    //Incomplete, needs TransformComponent updates.
    void GameEntityContextComponent::SetGameEntityParent(const AZ::EntityId& targetEntityId, const AZ::EntityId& parentEntityId)
    {
        if(!targetEntityId.IsValid()) { return; }

        AZ::Entity* e = FindEntity(targetEntityId);
        AZ::Entity* parentE;

        if(!e) { return; }

        //Determine which arrangement of active states is going on.
        AZ::u32 fromTo = 0; //0 active > Active, 1 Inactive -> Active, 2 Active -> Inactive, 3 Inactive -> Inactive

        if(parentEntityId.IsValid())
        {
            parentE = FindEntity(parentEntityId);

            if(!parentE) { return; }

            bool childActive = e->IsEffectivelyActive();
            bool parentActive = parentE->IsEffectivelyActive();

            if(childActive && parentActive)
            {
                fromTo = 0;
            }
            else if(!childActive && parentActive)
            {
                fromTo = 1;
            }
            else if(childActive && !parentActive)
            {
                fromTo = 2;
            }
            else
            {
                fromTo = 3;
            }

            // Orchestrate reparenting manually between parent and child.
            if (auto* tc = e->FindComponent<AzFramework::TransformComponent>())
            {
                auto* ptc = parentE->FindComponent<AzFramework::TransformComponent>();

                if(!ptc) { return; }

                switch (fromTo)
                {
                    case 0:
                        tc->SetParent(parentEntityId);
                        break;
                    
                    case 1:
                        ActivateGameEntityAndDescendants(targetEntityId, false);
                        tc->SetParent(parentEntityId);
                        break;
                    
                    case 2:
                        tc->SetInactiveParent(parentEntityId);
                        DeactivateGameEntityAndDescendants(targetEntityId, false);
                        break;
                    
                    case 3:
                        tc->SetInactiveParent(parentEntityId);
                        break;
                }
            }
        }
        else
        {
            bool childActive = e->IsEffectivelyActive();

            if(childActive)
            {
                fromTo = 0;
            }
            else
            {
                fromTo = 1;
            }

            
            // Orchestrate reparenting manually between child and none.
            if (auto* tc = e->FindComponent<AzFramework::TransformComponent>())
            {
                switch (fromTo)
                {
                    case 0:
                        tc->SetParent(parentEntityId);
                        break;
                    
                    case 1:
                        ActivateGameEntityAndDescendants(targetEntityId, false);
                        tc->SetParent(parentEntityId);
                        break;
                }
            }
        }
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

    
    
    //=========================================================================
    // AddEntityToParentChildTree
    //=========================================================================
    void GameEntityContextComponent::AddEntityToParentChildTree(AZ::Entity* entity)
    {
        if(!entity) { return; }
        
        //Get Parent
        AZ::EntityId parentId; // default = null root
        if (auto* tc = entity->FindComponent<AzFramework::TransformComponent>())
        {
            parentId = tc->GetParentId(); // works without bus
        }
        
        //Add parent to Child -> Parent tree.
        const AZ::EntityId id = entity->GetId();
        parentOf[id] = parentId;

        //Add child to Parent -> [Children..] tree
        //Returns a ref to the parent->vector<AZ::EntityId>
        auto& siblings = EnsureChildList(childrenByParentTree, parentId);

        //Check for duplicates. Then adds child to vector.
        if (AZStd::find(siblings.begin(), siblings.end(), id) == siblings.end())
        {
            siblings.push_back(id);
        }

        // Connect to TransformNotificationBus for this entity.
        if (!AZ::TransformNotificationBus::MultiHandler::BusIsConnectedId(id))
        {
            AZ::TransformNotificationBus::MultiHandler::BusConnect(id);
        }
    }

    //=========================================================================
    // RemoveEntityFromParentChildTree
    //=========================================================================
    void GameEntityContextComponent::RemoveEntityFromParentChildTree(AZ::Entity* entity)
    {
        if(!entity) { return; }
        
        RemoveEntityFromParentChildTreeById(entity->GetId());
    }

    //=========================================================================
    // RemoveEntityFromParentChildTreeById
    //=========================================================================
    void GameEntityContextComponent::RemoveEntityFromParentChildTreeById(const AZ::EntityId& entityId)
    {
        // Disconnect from TransformNotificationBus for this entity.
        if (AZ::TransformNotificationBus::MultiHandler::BusIsConnectedId(entityId))
        {
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(entityId);
        }

        // Find parent quickly from reverse map. If unknown, we're done.
        auto pIt = parentOf.find(entityId);
        if (pIt != parentOf.end())
        {
            const AZ::EntityId parentId = pIt->second;

            // Get child vector by parentId.
            auto cIt = childrenByParentTree.find(parentId);
            if (cIt != childrenByParentTree.end())
            {
                // If we found child vector, remove this ID from it.
                EraseValue(cIt->second, entityId);
                if (cIt->second.empty())
                {
                    // If vector is empty, parent has no children, remove the entire entry.
                    childrenByParentTree.erase(cIt);
                }
            }

            // Remove Child -> Parent entry, child no longer in tree.
            parentOf.erase(pIt);
        }

        // Reparent this entities children to null in the tree. Ideally transform parent change event will update the tree from there.
        auto asParent = childrenByParentTree.find(entityId);
        if (asParent != childrenByParentTree.end())
        {
            // If parent exists in tree. Get Children vector.
            auto& kids = asParent->second;
            if (!kids.empty())
            {
                // If children vector has children.
                // Reparent in our index (Transform system will do its own when active)
                for (const AZ::EntityId& kid : kids)
                {
                    parentOf[kid] = AZ::EntityId{}; // null
                }

                // Get children vector bound to 'null' parent.
                auto& rootKids = EnsureChildList(childrenByParentTree, AZ::EntityId{});
                // Add all of this entities children to 'null' parent.
                rootKids.insert(rootKids.end(), kids.begin(), kids.end());
            }

            // Remove this entity from parent tree, it is no longer part of the tree.
            childrenByParentTree.erase(asParent);
        }
    }
    
    //=========================================================================
    // GetEntityTreeFromRootEntity
    //=========================================================================
    void GameEntityContextComponent::GetEntityTreeFromRootEntity(const AZ::EntityId& rootEntityId, AZStd::vector<AZ::EntityId>& out)
    {
        const size_t start = out.size();   // allow append semantics
        out.push_back(rootEntityId);

        for (size_t i = start; i < out.size(); ++i)
        {
            const AZ::EntityId cur = out[i];
            if (auto it = childrenByParentTree.find(cur); it != childrenByParentTree.end())
            {
                const auto& kids = it->second;
                out.insert(out.end(), kids.begin(), kids.end());
            }
        }
    }
    
    //=========================================================================
    // PruneDescendantsFromTreeInPlace
    //=========================================================================
    // Remove every descendant of `root` from `list`, but only in the range (fromIndex, end].
    // Keeps order of the remaining items and shrinks the vector in-place.
    void GameEntityContextComponent::PruneDescendantsFromTreeInPlace(
        const AZ::EntityId& root,
        AZStd::vector<AZ::EntityId>& list,
        size_t fromIndex) const
    {
        // 1) Collect the subtree (excluding the root) using your adjacency map.
        AZStd::unordered_set<AZ::EntityId> toRemove;
        toRemove.reserve(64);

        AZStd::vector<AZ::EntityId> stack;
        // seed with root's children
        if (auto it = childrenByParentTree.find(root); it != childrenByParentTree.end())
        {
            const auto& kids = it->second;
            stack.insert(stack.end(), kids.begin(), kids.end());
        }

        while (!stack.empty())
        {
            AZ::EntityId cur = stack.back();
            stack.pop_back();

            if (toRemove.insert(cur).second) // newly inserted
            {
                if (auto it = childrenByParentTree.find(cur); it != childrenByParentTree.end())
                {
                    const auto& kids = it->second;
                    stack.insert(stack.end(), kids.begin(), kids.end());
                }
            }
        }

        // 2) Stable in-place filter: copy-compact anything not in 'toRemove'
        size_t write = fromIndex + 1;                       // we never touch elements <= fromIndex
        for (size_t read = fromIndex + 1; read < list.size(); ++read)
        {
            if (toRemove.find(list[read]) == toRemove.end())
            {
                list[write++] = list[read];
            }
        }
        list.resize(write);
    }
    
    //=========================================================================
    // RecomputeEffectiveActivationForEntity
    //=========================================================================
    void GameEntityContextComponent::RecomputeEffectiveActivationForEntity(AZ::Entity* movedChildEntity)
    {
        AZ::EntityId movedChild = movedChildEntity->GetId();
        bool childEffective = movedChildEntity->IsEffectivelyActive();

        bool incomingParentEffective = true;
        if (auto it = parentOf.find(movedChild); it != parentOf.end() && it->second.IsValid())
        {
            if (AZ::Entity* p = FindEntity(it->second))
            {
                incomingParentEffective = p->IsEffectivelyActive();
            }
        }

        if (incomingParentEffective && !childEffective)
        {
            // Parent is effectively active => propagate ON with parent-first order.
            // Skip change local state in order to preserve that this is simply reacting to parent state.
            ActivateGameEntityAndDescendants(movedChild, false);
        }
        else if (!incomingParentEffective && childEffective)
        {
            // Parent is effectively inactive => force subtree OFF (children-first)
            // Skip change local state in order to preserve that this is simply reacting to parent state.
            DeactivateGameEntityAndDescendants(movedChild, false);
        }
    }

    //=========================================================================
    // TransformNotificationBus::OnParentChanged
    //=========================================================================
    void GameEntityContextComponent::OnParentChanged(AZ::EntityId oldParentId, AZ::EntityId newParentId)
    {
        const AZ::EntityId entityId = *AZ::TransformNotificationBus::GetCurrentBusId();

        UpdateParentChildMaps(entityId, oldParentId, newParentId);

        AZ::Entity* e = FindEntity(entityId);

        //Filter for startup parenting. No need to recompute.
        AZ::Entity::State state = e->GetState();
        if( state == AZ::Entity::State::Init || state == AZ::Entity::State::Active )
        {
            RecomputeEffectiveActivationForEntity(e);
        }
    }

    //=========================================================================
    // UpdateParentChildMaps
    //=========================================================================
    void GameEntityContextComponent::UpdateParentChildMaps(AZ::EntityId child, AZ::EntityId oldParent, AZ::EntityId newParent)
    {
        // remove from old parent's list
        if (auto it = childrenByParentTree.find(oldParent); it != childrenByParentTree.end())
        {
            EraseValue(it->second, child);
            if (it->second.empty()) { childrenByParentTree.erase(it); }
        }

        // add to new parent's list
        auto& newSiblings = EnsureChildList(childrenByParentTree, newParent);
        if (AZStd::find(newSiblings.begin(), newSiblings.end(), child) == newSiblings.end())
        {
            newSiblings.push_back(child);
        }

        // reverse index
        parentOf[child] = newParent;

        // (optional) recompute effective activation for the moved subtree
        // RecomputeEffectiveActivation(child);
    }
} // namespace AzFramework
