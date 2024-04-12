/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>

#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipService.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Undo/UndoCacheInterface.h>

DECLARE_EBUS_INSTANTIATION(AzToolsFramework::EditorEntityContextRequests);

namespace AzToolsFramework
{
    namespace Internal
    {
        struct EditorEntityContextNotificationBusHandler final
            : public EditorEntityContextNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
            AZ_EBUS_BEHAVIOR_BINDER(EditorEntityContextNotificationBusHandler, "{159C07A6-BCB6-432E-BEBB-6AABF6C76989}", AZ::SystemAllocator,
                OnEditorEntityCreated, OnEditorEntityDeleted);

            void OnEditorEntityCreated(const AZ::EntityId& entityId) override
            {
                Call(FN_OnEditorEntityCreated, entityId);
            }

            void OnEditorEntityDeleted(const AZ::EntityId& entityId) override
            {
                Call(FN_OnEditorEntityDeleted, entityId);
            }
        };
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void EditorEntityContextComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorEntityContextComponent, AZ::Component>()
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorEntityContextComponent>(
                    "Editor Entity Context", "System component responsible for owning the edit-time entity context")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorEntityContextRequestBus>("EditorEntityContextRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor")
                ->Event("GetEditorEntityContextId", &EditorEntityContextRequests::GetEditorEntityContextId)
                ;

            behaviorContext->EBus<EditorEntityContextNotificationBus>("EditorEntityContextNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor")
                ->Handler<Internal::EditorEntityContextNotificationBusHandler>()
                ->Event("OnEditorEntityCreated", &EditorEntityContextNotification::OnEditorEntityCreated)
                ->Event("OnEditorEntityDeleted", &EditorEntityContextNotification::OnEditorEntityDeleted)
                ;
        }
    }

    //=========================================================================
    // EditorEntityContextComponent ctor
    //=========================================================================
    EditorEntityContextComponent::EditorEntityContextComponent()
        : AzFramework::EntityContext(AzFramework::EntityContextId::CreateRandom())
        , m_isRunningGame(false)
        , m_requiredEditorComponentTypes
        // These are the components that will be force added to every entity in the editor
        ({
            azrtti_typeid<AzToolsFramework::Components::EditorDisabledCompositionComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorOnlyEntityComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorEntityIconComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorEntitySortComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorInspectorComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorLockComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorPendingCompositionComponent>(),
            azrtti_typeid<AzToolsFramework::Components::EditorVisibilityComponent>(),
            azrtti_typeid<AzToolsFramework::Components::TransformComponent>()
    })
    {
    }

    //=========================================================================
    // EditorEntityContextComponent dtor
    //=========================================================================
    EditorEntityContextComponent::~EditorEntityContextComponent()
    {
    }

    //=========================================================================
    // Init
    //=========================================================================
    void EditorEntityContextComponent::Init()
    {
    }

    //=========================================================================
    // Activate
    //=========================================================================
    void EditorEntityContextComponent::Activate()
    {
        m_entityOwnershipService = AZStd::make_unique<PrefabEditorEntityOwnershipService>(GetContextId(), GetSerializeContext());

        InitContext();

        m_entityOwnershipService->InstantiateAllPrefabs();

        EditorEntityContextRequestBus::Handler::BusConnect();
        EditorEntityContextPickingRequestBus::Handler::BusConnect(GetContextId());
        EditorLegacyGameModeNotificationBus::Handler::BusConnect();
    }

    //=========================================================================
    // Deactivate
    //=========================================================================
    void EditorEntityContextComponent::Deactivate()
    {
        EditorLegacyGameModeNotificationBus::Handler::BusDisconnect();
        EditorEntityContextRequestBus::Handler::BusDisconnect();
        EditorEntityContextPickingRequestBus::Handler::BusDisconnect();

        DestroyContext();

        m_entityOwnershipService.reset();
    }

    //=========================================================================
    // EditorEntityContextRequestBus::ResetEditorContext
    //=========================================================================
    void EditorEntityContextComponent::ResetEditorContext()
    {
        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotificationBus::Events::OnPrepareForContextReset);

        if (m_isRunningGame)
        {
            // Ensure we exit play-in-editor when the context is reset (switching levels).
            StopPlayInEditor();
        }

        ResetContext();

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotificationBus::Events::OnContextReset);
    }

    //=========================================================================
    // EditorEntityContextRequestBus::CreateEditorEntity
    //=========================================================================
    AZ::EntityId EditorEntityContextComponent::CreateNewEditorEntity(const char* name)
    {
        AZ::Entity* entity = CreateEntity(name);
        AZ_Assert(entity != nullptr, "Entity with name %s couldn't be created.", name);
        FinalizeEditorEntity(entity);

        return entity->GetId();
    }

    //=========================================================================
    // EditorEntityContextRequestBus::CreateEditorEntityWithId
    //=========================================================================
    AZ::EntityId EditorEntityContextComponent::CreateNewEditorEntityWithId(const char* name, const AZ::EntityId& entityId)
    {
        if (!entityId.IsValid())
        {
            AZ_Warning("EditorEntityContextComponent", false, "Cannot create an entity with an invalid ID.");
            return AZ::EntityId();
        }
        // Make sure this ID is not already in use.
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        if (entity)
        {
            AZ_Warning(
                "EditorEntityContextComponent",
                false,
                "An entity already exists with ID %s, a new entity will not be created.",
                entityId.ToString().c_str());
            return AZ::EntityId();
        }
        entity = aznew AZ::Entity(entityId, name);
        AZ_Assert(entity != nullptr, "Entity with name %s couldn't be created.", name);
        AddEntity(entity);
        FinalizeEditorEntity(entity);

        return entity->GetId();
    }

    //=========================================================================
    // EditorEntityContextComponent::FinalizeEditorEntity
    //=========================================================================
    void EditorEntityContextComponent::FinalizeEditorEntity(AZ::Entity* entity)
    {
        if (!entity)
        {
            return;
        }

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEditorEntityCreated, entity->GetId());
    }

    //=========================================================================
    // EditorEntityContextRequestBus::AddEditorEntity
    //=========================================================================
    void EditorEntityContextComponent::AddEditorEntity(AZ::Entity* entity)
    {
        AZ_Assert(entity, "Supplied entity is invalid.");

        AddEntity(entity);
    }

    //=========================================================================
    // EditorEntityContextRequestBus::AddEditorEntities
    //=========================================================================
    void EditorEntityContextComponent::AddEditorEntities(const EntityList& entities)
    {
        m_entityOwnershipService->AddEntities(entities);
    }

    void EditorEntityContextComponent::HandleEntitiesAdded(const EntityList& entities)
    {
        m_entityOwnershipService->HandleEntitiesAdded(entities);
    }

    //=========================================================================
    // EditorEntityContextRequestBus::CloneEditorEntities
    //=========================================================================
    bool EditorEntityContextComponent::CloneEditorEntities(const EntityIdList& sourceEntities,
                                                           EntityList& resultEntities,
                                                           EntityIdToEntityIdMap& sourceToCloneEntityIdMap)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        resultEntities.clear();

        AZ::EntityUtils::SerializableEntityContainer sourceObjects;
        for (const AZ::EntityId& id : sourceEntities)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, id);
            if (entity)
            {
                sourceObjects.m_entities.push_back(entity);
            }
        }

        AZ::EntityUtils::SerializableEntityContainer* clonedObjects =
            AZ::IdUtils::Remapper<AZ::EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(&sourceObjects, sourceToCloneEntityIdMap);
        if (!clonedObjects)
        {
            AZ_Error("EditorEntityContext", false, "Failed to clone source entities.");
            sourceToCloneEntityIdMap.clear();
            return false;
        }

        resultEntities = clonedObjects->m_entities;

        delete clonedObjects;

        return true;
    }

    //=========================================================================
    // EditorEntityContextRequestBus::DestroyEditorEntity
    //=========================================================================
    bool EditorEntityContextComponent::DestroyEditorEntity(AZ::EntityId entityId)
    {
        if (DestroyEntityById(entityId))
        {
            return true;
        }

        return false;
    }

    //=========================================================================
    // EditorEntityContextRequestBus::SaveToStreamForEditor
    //=========================================================================
    bool EditorEntityContextComponent::SaveToStreamForEditor(
        AZ::IO::GenericStream& /* stream */,
        const EntityList& /* entitiesInLayers */,
        AZ::SliceComponent::SliceReferenceToInstancePtrs& /* instancesInLayers */)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ_Assert(!m_entityOwnershipService->m_shouldAssertForLegacySlicesUsage, "Not implemented");
        return true;
    }

    void EditorEntityContextComponent::GetLooseEditorEntities(EntityList& entityList)
    {
        m_entityOwnershipService->GetNonPrefabEntities(entityList);
    }

    //=========================================================================
    // EditorEntityContextRequestBus::SaveToStreamForGame
    //=========================================================================
    bool EditorEntityContextComponent::SaveToStreamForGame(AZ::IO::GenericStream& /* stream */, AZ::DataStream::StreamType /* streamType */)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ_Assert(!m_entityOwnershipService->m_shouldAssertForLegacySlicesUsage, "Not implemented");
        return true;
    }

    //=========================================================================
    // EditorEntityContextRequestBus::LoadFromStream
    //=========================================================================
    bool EditorEntityContextComponent::LoadFromStream(AZ::IO::GenericStream& stream)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ_Assert(stream.IsOpen(), "Invalid source stream.");
        AZ_Assert(m_entityOwnershipService->IsInitialized(), "The context has not been initialized.");

        EditorEntityContextNotificationBus::Broadcast(
            &EditorEntityContextNotification::OnEntityStreamLoadBegin);

        const bool loadedSuccessfully = m_entityOwnershipService->LoadFromStream(stream, false, nullptr,
            AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterSourceSlicesOnly));

        LoadFromStreamComplete(loadedSuccessfully);

        return loadedSuccessfully;
    }

    bool EditorEntityContextComponent::LoadFromStreamWithLayers(AZ::IO::GenericStream& stream, QString levelPakFile)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ_Assert(stream.IsOpen(), "Invalid source stream.");
        AZ_Assert(m_entityOwnershipService->IsInitialized(), "The context has not been initialized.");

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEntityStreamLoadBegin);

        bool loadedSuccessfully = static_cast<PrefabEditorEntityOwnershipService*>(m_entityOwnershipService.get())->LoadFromStream(
                stream, AZStd::string_view(levelPakFile.toUtf8().constData(), levelPakFile.size()) );
        
        LoadFromStreamComplete(loadedSuccessfully);
        
        return loadedSuccessfully;
    }
    
    void EditorEntityContextComponent::LoadFromStreamComplete(bool loadedSuccessfully)
    {
        if (loadedSuccessfully)
        {
            EntityList entities;
            m_entityOwnershipService->GetAllEntities(entities);

            AzFramework::SliceEntityOwnershipServiceRequestBus::Event(GetContextId(),
                &AzFramework::SliceEntityOwnershipServiceRequests::SetIsDynamic, true);

            SetupEditorEntities(entities);
            EditorEntityContextNotificationBus::Broadcast(
                &EditorEntityContextNotification::OnEntityStreamLoadSuccess);
        }
        else
        {
            EditorEntityContextNotificationBus::Broadcast(
                &EditorEntityContextNotification::OnEntityStreamLoadFailed);
        }
    }

    //=========================================================================
    // EntityContextRequestBus::StartPlayInEditor
    //=========================================================================
    void EditorEntityContextComponent::StartPlayInEditor()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnStartPlayInEditorBegin);

        //cache the current selected entities.
        ToolsApplicationRequests::Bus::BroadcastResult(m_selectedBeforeStartingGame, &ToolsApplicationRequests::GetSelectedEntities);
        //deselect entities if selected when entering game mode before deactivating the entities in StartPlayInEditor(...)
        if (!m_selectedBeforeStartingGame.empty())
        {
            ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::MarkEntitiesDeselected, m_selectedBeforeStartingGame);
        }

        auto* service = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
        AZ_Assert(service, "Start play in editor could not start because there was no implementation for "
            "PrefabEditorEntityOwnershipInterface");
        service->StartPlayInEditor();

        m_isRunningGame = true;

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnStartPlayInEditor);
    }

    //=========================================================================
    // EntityContextRequestBus::StopPlayInEditor
    //=========================================================================
    void EditorEntityContextComponent::StopPlayInEditor()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnStopPlayInEditorBegin);

        m_isRunningGame = false;

        auto* service = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
        AZ_Assert(service, "Stop play in editor could not complete because there was no implementation for "
            "PrefabEditorEntityOwnershipInterface");
        service->StopPlayInEditor();

        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, m_selectedBeforeStartingGame);
        m_selectedBeforeStartingGame.clear();

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnStopPlayInEditor);
    }

    //=========================================================================
    // EntityContextRequestBus::IsEditorRunningGame
    //=========================================================================
    bool EditorEntityContextComponent::IsEditorRunningGame()
    {
        return m_isRunningGame;
    }

    bool EditorEntityContextComponent::IsEditorRequestingGame()
    {
        return m_isRequestingGame;
    }

    //=========================================================================
    // EntityContextRequestBus::IsEditorEntity
    //=========================================================================
    bool EditorEntityContextComponent::IsEditorEntity(AZ::EntityId id)
    {
        AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(contextId, id, &EntityIdContextQueries::GetOwningContextId);

        if (contextId == GetContextId())
        {
            return true;
        }

        return false;
    }

    //=========================================================================
    // EntityContextRequestBus::AddRequiredComponents
    //=========================================================================
    void EditorEntityContextComponent::AddRequiredComponents(AZ::Entity& entity)
    {
        for (const auto& componentType : m_requiredEditorComponentTypes)
        {
            if (!entity.FindComponent(componentType))
            {
                entity.CreateComponent(componentType);
            }
        }
    }

    //=========================================================================
    // EntityContextRequestBus::GetRequiredComponentTypes
    //=========================================================================
    const AZ::ComponentTypeList& EditorEntityContextComponent::GetRequiredComponentTypes()
    {
        return m_requiredEditorComponentTypes;
    }

    //=========================================================================
    // EntityContextEventBus::MapEditorIdToRuntimeId
    //=========================================================================
    bool EditorEntityContextComponent::MapEditorIdToRuntimeId(const AZ::EntityId& editorId, AZ::EntityId& runtimeId)
    {
        auto iter = m_editorToRuntimeIdMap.find(editorId);
        if (iter != m_editorToRuntimeIdMap.end())
        {
            runtimeId = iter->second;
            return true;
        }

        return false;
    }

    //=========================================================================
    // EntityContextEventBus::MapRuntimeIdToEditorId
    //=========================================================================
    bool EditorEntityContextComponent::MapRuntimeIdToEditorId(const AZ::EntityId& runtimeId, AZ::EntityId& editorId)
    {
        auto iter = m_runtimeToEditorIdMap.find(runtimeId);
        if (iter != m_runtimeToEditorIdMap.end())
        {
            editorId = iter->second;
            return true;
        }

        return false;
    }

    //=========================================================================
    // EditorEntityContextPickingRequestBus::SupportsViewportEntityIdPicking
    //=========================================================================
    bool EditorEntityContextComponent::SupportsViewportEntityIdPicking()
    {
        return true;
    }

    //=========================================================================
    // PrepareForContextReset
    //=========================================================================
    void EditorEntityContextComponent::PrepareForContextReset()
    {
        EntityContext::PrepareForContextReset();
        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnPrepareForContextReset);
    }

    //=========================================================================
    // ValidateEntitiesAreValidForContext
    //=========================================================================
    bool EditorEntityContextComponent::ValidateEntitiesAreValidForContext(const EntityList& entities)
    {
        // All entities in a prefab being instantiated in the level editor should
        // have the TransformComponent on them. Since it is not possible to create
        // a prefab with entities from different contexts, it is OK to check
        // the first entity only
        if (entities.size() > 0)
        {
            return entities[0]->FindComponent<Components::TransformComponent>() != nullptr;
        }

        return true;
    }

    //=========================================================================
    // OnContextEntitiesAdded
    //=========================================================================
    void EditorEntityContextComponent::OnContextEntitiesAdded(const EntityList& entities)
    {
        EntityContext::OnContextEntitiesAdded(entities);

        SetupEditorEntities(entities);
    }

    //=========================================================================
    // OnContextEntityRemoved
    //=========================================================================
    void EditorEntityContextComponent::OnContextEntityRemoved(const AZ::EntityId& entityId)
    {
        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEditorEntityDeleted, entityId);
    }

    //=========================================================================
    // SetupEditorEntity
    //=========================================================================
    void EditorEntityContextComponent::SetupEditorEntity(AZ::Entity* entity)
    {
        SetupEditorEntities({ entity });
    }

    //=========================================================================
    // SetupEditorEntities
    //=========================================================================
    void EditorEntityContextComponent::SetupEditorEntities(const EntityList& entities)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ::Data::AssetManager::Instance().SuspendAssetRelease();

        // All editor entities are automatically activated.
        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "EditorEntityContextComponent::SetupEditorEntities:ScrubEntities");

            // Scrub entities before initialization.
            // Anything could go wrong with entities loaded from disk.
            // Ex: There might be duplicates of components that do not tolerate
            // duplication and would crash during their Init().
            EntityCompositionRequestBus::Broadcast(&EntityCompositionRequestBus::Events::ScrubEntities, entities);
        }

        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "EditorEntityContextComponent::SetupEditorEntities:InitEntities");
            for (AZ::Entity* entity : entities)
            {
                if (entity->GetState() == AZ::Entity::State::Constructed)
                {
                    entity->Init();
                }
            }
        }

        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "EditorEntityContextComponent::SetupEditorEntities:CreateEditorRepresentations");
            for (AZ::Entity* entity : entities)
            {
                EditorRequests::Bus::Broadcast(&EditorRequests::CreateEditorRepresentation, entity);
            }
        }

        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "EditorEntityContextComponent::SetupEditorEntities:ActivateEntities");
            for (AZ::Entity* entity : entities)
            {
                if (entity->GetState() == AZ::Entity::State::Init)
                {
                    // Always invalidate the entity dependencies when loading in the editor
                    // (we don't know what code has changed since the last time the editor was run and the services provided/required
                    // by entities might have changed)
                    entity->InvalidateDependencies();
                    entity->Activate();
                }
            }

            if (m_undoCacheInterface == nullptr)
            {
                m_undoCacheInterface = AZ::Interface<UndoSystem::UndoCacheInterface>::Get();
            }

            // After activating all the entities, refresh their entries in the undo cache.  
            // We need to wait until after all the activations are complete.  Otherwise, it's possible that the data for an entity
            // will change based on other activations.  For example, if we activate a child entity before its parent, the Transform
            // component on the child will refresh its cache state on the parent activation.  This would cause the undo cache to be
            // out of sync with the child data.
            for (AZ::Entity* entity : entities)
            {
                m_undoCacheInterface->UpdateCache(entity->GetId());
            }
        }

        AZ::Data::AssetManager::Instance().ResumeAssetRelease();
    }

    void EditorEntityContextComponent::OnStartGameModeRequest()
    {
        m_isRequestingGame = true;
    }

    void EditorEntityContextComponent::OnStopGameModeRequest()
    {
        m_isRequestingGame = false;
    }

} // namespace AzToolsFramework
