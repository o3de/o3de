/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/containers/stack.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

#include "EntityContext.h"

namespace AzFramework
{
    //=========================================================================
    // Reflect
    //=========================================================================
    void EntityContext::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // EntityContext entity data is serialized through streams / Ebus messages.
            serializeContext->Class<EntityContext>()
                ->Version(1)
                ;
        }
    }

    AZStd::shared_ptr<Scene> EntityContext::FindContainingScene(const EntityContextId& contextId)
    {
        auto sceneSystem = SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "Attempting to retrieve the scene containing a entity context before the scene system is available.");

        AZStd::shared_ptr<Scene> result;
        sceneSystem->IterateActiveScenes([&result, &contextId](const AZStd::shared_ptr<Scene>& scene)
            {
                EntityContext** entityContext = scene->FindSubsystemInScene<EntityContext::SceneStorageType>();
                if (entityContext && (*entityContext)->GetContextId() == contextId)
                {
                    result = scene;
                    // Result found, returning.
                    return false;
                }
                else
                {
                    // No match, continuing to search for containing scene.
                    return true;
                }
            });
        return result;
    }

    //=========================================================================
    // EntityContext ctor
    //=========================================================================
    EntityContext::EntityContext(AZ::SerializeContext* serializeContext /*= nullptr*/)
        : EntityContext(EntityContextId::CreateRandom(), serializeContext)
    {
        EntityContextRequestBus::Handler::BusConnect(m_contextId);
    }

    //=========================================================================
    // EntityContext ctor
    //=========================================================================
    EntityContext::EntityContext(const AZ::Uuid& contextId, AZ::SerializeContext* serializeContext /*= nullptr*/)
        : EntityContext(contextId, nullptr, serializeContext)
    {  
    }

    EntityContext::EntityContext(const EntityContextId& contextId, AZStd::unique_ptr<EntityOwnershipService> entityOwnershipService,
        AZ::SerializeContext* serializeContext)
        : m_serializeContext(serializeContext)
        , m_contextId(contextId)
    {
        if (nullptr == serializeContext)
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve application serialization context.");
        }
        if (m_contextId.IsNull())
        {
            m_contextId = EntityContextId::CreateRandom();
            AZ_Assert(m_contextId.IsNull(), "Failed to create an entity context id.");
        }
        if (nullptr == entityOwnershipService)
        {
            m_entityOwnershipService = AZStd::make_unique<AzFramework::SliceEntityOwnershipService>(m_contextId, m_serializeContext);
            AZ_Assert(m_entityOwnershipService, "Failed to create an entity ownership service.");
        }
        else
        {
            m_entityOwnershipService = AZStd::move(entityOwnershipService);
        }
        EntityContextRequestBus::Handler::BusConnect(m_contextId);
        EntityContextEventBus::Bind(m_eventBusPtr, m_contextId);
    }

    //=========================================================================
    // EntityContext dtor
    //=========================================================================
    EntityContext::~EntityContext()
    {
        m_eventBusPtr = nullptr;
        DestroyContext();
    }

    //=========================================================================
    // InitContext
    //=========================================================================
    void EntityContext::InitContext()
    {
        AZ_Assert(m_entityOwnershipService, "Entity Ownership Service has not been created yet");
        EntityOwnershipServiceNotificationBus::Handler::BusConnect(m_contextId);
        m_entityOwnershipService->Initialize();

        // If any of the entity contexts that extend the base entity context override these handler functions, those overriden functions
        // will be set as the callbacks.
        m_entityOwnershipService->SetEntitiesAddedCallback([this](const EntityList& entityList)
        {
            this->HandleEntitiesAdded(entityList);
        });

        m_entityOwnershipService->SetEntitiesRemovedCallback([this](const EntityIdList& entityIds)
        {
            this->HandleEntitiesRemoved(entityIds);
        });

        m_entityOwnershipService->SetValidateEntitiesCallback([this](const EntityList& entities)
        {
            return this->ValidateEntitiesAreValidForContext(entities);
        });
    }

    //=========================================================================
    // DestroyContext
    //=========================================================================
    void EntityContext::DestroyContext()
    {
        if (m_entityOwnershipService)
        {
            m_entityOwnershipService->Reset();
            EntityOwnershipServiceNotificationBus::Handler::BusDisconnect(m_contextId);
            m_entityOwnershipService->Destroy();
        }
    }

    //=========================================================================
    // ResetContext
    //=========================================================================
    void EntityContext::ResetContext()
    {
        m_entityOwnershipService->Reset();
    }


    //=========================================================================
    // HandleEntitiesAdded
    //=========================================================================
    void EntityContext::HandleEntitiesAdded(const EntityList& entities)
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        for (AZ::Entity* entity : entities)
        {
            AZ::EntityBus::MultiHandler::BusConnect(entity->GetId());
            EntityIdContextQueryBus::MultiHandler::BusConnect(entity->GetId());
            EntityContextEventBus::Event(m_eventBusPtr, &EntityContextEventBus::Events::OnEntityContextCreateEntity, *entity);
        }

        OnContextEntitiesAdded(entities);
    }

    //=========================================================================
    // HandleEntitiesRemoved
    //=========================================================================
    void EntityContext::HandleEntitiesRemoved(const EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        for (AZ::EntityId id : entityIds)
        {
            OnContextEntityRemoved(id);

            EntityContextEventBus::Event(m_eventBusPtr, &EntityContextEventBus::Events::OnEntityContextDestroyEntity, id);
            EntityIdContextQueryBus::MultiHandler::BusDisconnect(id);
            AZ::EntityBus::MultiHandler::BusDisconnect(id);
        }
    }

    //=========================================================================
    // ValidateEntitiesAreValidForContext
    //=========================================================================
    bool EntityContext::ValidateEntitiesAreValidForContext(const EntityList&)
    {
        return true;
    }

    //=========================================================================
    // IsOwnedByThisContext
    //=========================================================================
    bool EntityContext::IsOwnedByThisContext(const AZ::EntityId& entityId)
    {
        // Get ID of the owning context of the incoming entity ID and compare it to
        // the id of this context.
        EntityContextId owningContextId = EntityContextId::CreateNull();
        EntityIdContextQueryBus::EventResult(owningContextId, entityId, &EntityIdContextQueryBus::Events::GetOwningContextId);
        return owningContextId == m_contextId;
    }

    //=========================================================================
    // CreateEntity
    //=========================================================================
    AZ::Entity* EntityContext::CreateEntity(const char* name)
    {
        AZ::Entity* entity = aznew AZ::Entity(name);

        AddEntity(entity);

        return entity;
    }

    //=========================================================================
    // AddEntity
    //=========================================================================
    void EntityContext::AddEntity(AZ::Entity* entity)
    {
        AZ_Assert(!EntityIdContextQueryBus::FindFirstHandler(entity->GetId()), "Entity already belongs to a context.");

        m_entityOwnershipService->AddEntity(entity);
    }

    //=========================================================================
    // ActivateEntity
    //=========================================================================
    void EntityContext::ActivateEntity(AZ::EntityId entityId)
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
                    entity->Activate();
                }
            }
        }
    }

    //=========================================================================
    // DeactivateEntity
    //=========================================================================
    void EntityContext::DeactivateEntity(AZ::EntityId entityId)
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
                    AZ::TickBus::QueueFunction(&AZ::Entity::Deactivate, entity);
                    break;

                case AZ::Entity::State::Active:
                    // Deactivate immediately
                    entity->Deactivate();
                    break;

                default:
                    // Don't do anything, it's not even active.
                    break;
                }
            }
        }
    }

    //=========================================================================
    // DestroyEntity
    //=========================================================================
    bool EntityContext::DestroyEntity(AZ::Entity* entity)
    {
        AZ_Assert(entity, "Invalid entity passed to DestroyEntity");

        EntityContextId owningContextId = EntityContextId::CreateNull();
        EntityIdContextQueryBus::EventResult(owningContextId, entity->GetId(), &EntityIdContextQueryBus::Events::GetOwningContextId);
        AZ_Assert(owningContextId == m_contextId, "Entity does not belong to this context, and therefore can not be safely destroyed by this context.");

        if (owningContextId == m_contextId)
        {
            return m_entityOwnershipService->DestroyEntity(entity);
        }

        return false;
    }

    //=========================================================================
    // DestroyEntity
    //=========================================================================
    bool EntityContext::DestroyEntityById(AZ::EntityId entityId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

        if (entity)
        {
            return DestroyEntity(entity);
        }

        return false;
    }

    //=========================================================================
    // CloneEntity
    //=========================================================================
    AZ::Entity* EntityContext::CloneEntity(const AZ::Entity& sourceEntity)
    {
        AZ_Assert(m_entityOwnershipService->IsInitialized(), "The context has not been initialized.");

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve application serialization context.");

        AZ::Entity* entity = serializeContext->CloneObject(&sourceEntity);
        AZ_Error("EntityContext", entity != nullptr, "Failed to clone source entity.");

        if (entity)
        {
            entity->SetId(AZ::Entity::MakeId());
            AddEntity(entity);
        }

        return entity;
    }

    //=========================================================================
    // EntityBus::OnEntityDestruction
    //=========================================================================
    void EntityContext::OnEntityDestruction(const AZ::EntityId& entityId)
    {
        EntityContextId owningContextId = EntityContextId::CreateNull();
        EntityIdContextQueryBus::EventResult(owningContextId, entityId, &EntityIdContextQueryBus::Events::GetOwningContextId);
        if (owningContextId == m_contextId)
        {
            m_entityOwnershipService->HandleEntityBeingDestroyed(entityId);
        }
    }

    AZ::SerializeContext* EntityContext::GetSerializeContext() const
    {
        return m_serializeContext;
    }

    void EntityContext::PrepareForEntityOwnershipServiceReset()
    {
        PrepareForContextReset();
    }

    void EntityContext::OnEntityOwnershipServiceReset()
    {
        OnContextReset();
        EntityContextEventBus::Event(m_contextId, &EntityContextEventBus::Events::OnEntityContextReset);
    }

    void EntityContext::OnEntitiesReloadedFromStream(const EntityList& entities)
    {
        OnRootEntityReloaded();
        EntityContextEventBus::Event(m_contextId, &EntityContextEventBus::Events::OnEntityContextLoadedFromStream, entities);
    }
} // namespace AzFramework
