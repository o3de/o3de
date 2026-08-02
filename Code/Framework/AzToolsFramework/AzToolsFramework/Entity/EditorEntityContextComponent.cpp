/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>

#include <AzFramework/Translation/TranslationDef.h>

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
#include <AzCore/std/algorithm.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Render/Intersector.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

#include <AzToolsFramework/ToolsComponents/EditorEntityIconComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipService.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Undo/UndoCacheInterface.h>

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

    //! An additional editor world: a real entity context owning one level as its root prefab and
    //! living in its own scene (child of Main). Entities added here receive the same editor
    //! treatment as world 0's; a render scene is attached by the first viewport that binds.
    class EditorEntityContextComponent::EditorWorld
        : public AzFramework::EntityContext
    {
    public:
        EditorWorld(EditorEntityContextComponent& owner, AZ::IO::PathView levelPrefabPath)
            : AzFramework::EntityContext(AzFramework::EntityContextId::CreateRandom())
            , m_owner(owner)
        {
            m_entityOwnershipService = AZStd::make_unique<PrefabEditorEntityOwnershipService>(GetContextId(), GetSerializeContext());
            InitContext();

            // The scene (with this context as its subsystem) must exist before the level's entities
            // activate, so their render components resolve to this world's scene and not Main's.
            auto sceneSystem = AzFramework::SceneSystemInterface::Get();
            AZStd::shared_ptr<AzFramework::Scene> mainScene =
                sceneSystem ? sceneSystem->GetScene(AzFramework::Scene::MainSceneName) : nullptr;
            AZ_Assert(mainScene, "Editor world created without a main scene to parent its scene under");
            if (mainScene)
            {
                m_sceneName = AZStd::string::format("Editor World %s", GetContextId().ToFixedString().c_str());
                auto sceneOutcome = sceneSystem->CreateSceneWithParent(m_sceneName, mainScene);
                AZ_Assert(sceneOutcome.IsSuccess(), "Failed to create an editor world scene: %s",
                    sceneOutcome.GetError().c_str());
                if (sceneOutcome.IsSuccess())
                {
                    m_scene = sceneOutcome.GetValue();
                    AzFramework::EntityContext* worldContext = this;
                    [[maybe_unused]] const bool contextAdded =
                        m_scene->SetSubsystem<AzFramework::EntityContext::SceneStorageType&>(worldContext);
                    AZ_Assert(contextAdded, "Unable to add an editor world's entity context to its scene");
                }
            }

            m_intersector = AZStd::make_unique<AzFramework::RenderGeometry::Intersector>(GetContextId());

            auto* editorModeTracker = AZ::Interface<ViewportEditorModeTrackerInterface>::Get();
            AZ_Assert(editorModeTracker, "Editor world created before the viewport editor mode tracker");
            if (editorModeTracker)
            {
                editorModeTracker->ActivateMode({ GetContextId() }, ViewportEditorMode::Default);
            }

            // Only the level's template loads now; instantiation waits until a viewport has bound
            // and attached a render scene, or entities would resolve to the Main scene instead.
            auto* prefabLoader = AZ::Interface<Prefab::PrefabLoaderInterface>::Get();
            auto* prefabSystem = AZ::Interface<Prefab::PrefabSystemComponentInterface>::Get();
            AZ_Assert(prefabLoader && prefabSystem, "Editor world created without the prefab system");
            if (prefabLoader && prefabSystem)
            {
                // Template queries take project-relative paths.
                const AZ::IO::Path relativeLevelPath = prefabLoader->GenerateRelativePath(levelPrefabPath);
                Prefab::TemplateId templateId = prefabSystem->GetTemplateIdFromFilePath(relativeLevelPath);
                if (templateId == Prefab::InvalidTemplateId)
                {
                    templateId = prefabLoader->LoadTemplateFromFile(levelPrefabPath);
                }
                if (templateId != Prefab::InvalidTemplateId)
                {
                    m_levelPath = relativeLevelPath;
                    m_pendingLevelLoad = true;
                }
            }
        }

        bool LoadPendingLevel()
        {
            if (!m_pendingLevelLoad)
            {
                return false;
            }
            m_pendingLevelLoad = false;

            PrefabEditorEntityOwnershipInterface* prefabService = GetOwnershipService();
            Prefab::InstanceOptionalReference rootInstance = prefabService->LoadRootPrefab(m_levelPath);
            AZ_Error("EditorWorld", rootInstance.has_value(), "Could not instantiate the level '%s' in its world",
                m_levelPath.c_str());
            return rootInstance.has_value();
        }

        ~EditorWorld() override
        {
            if (auto* editorModeTracker = AZ::Interface<ViewportEditorModeTrackerInterface>::Get())
            {
                editorModeTracker->DeactivateMode({ GetContextId() }, ViewportEditorMode::Default);
            }

            m_intersector.reset();
            DestroyContext();
            m_entityOwnershipService.reset();

            if (m_scene)
            {
                m_scene.reset();
                AzFramework::SceneSystemInterface::Get()->RemoveScene(m_sceneName);
            }
        }

        bool IsValid() const
        {
            return !m_levelPath.empty();
        }

        const AZ::IO::Path& GetLevelPath() const
        {
            return m_levelPath;
        }

        AZStd::shared_ptr<AzFramework::Scene> GetScene() const
        {
            return m_scene;
        }

        PrefabEditorEntityOwnershipService* GetOwnershipService() const
        {
            return static_cast<PrefabEditorEntityOwnershipService*>(m_entityOwnershipService.get());
        }

    protected:
        void OnContextEntitiesAdded(const EntityList& entities) override
        {
            EntityContext::OnContextEntitiesAdded(entities);
            m_owner.SetupEditorEntities(entities);
        }

        void OnContextEntityRemoved(const AZ::EntityId& entityId) override
        {
            EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEditorEntityDeleted, entityId);
        }

    private:
        EditorEntityContextComponent& m_owner;
        AZStd::string m_sceneName;
        AZStd::shared_ptr<AzFramework::Scene> m_scene;
        AZStd::unique_ptr<AzFramework::RenderGeometry::Intersector> m_intersector;
        AZ::IO::Path m_levelPath;
        bool m_pendingLevelLoad = false;
    };

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
                    QT_TRANSLATE_NOOP("AzToolsFramework", "Editor Entity Context"),
                    QT_TRANSLATE_NOOP("AzToolsFramework", "System component responsible for owning the edit-time entity context"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, QT_TRANSLATE_NOOP("AzToolsFramework", "Editor"))
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

        m_viewportWorlds.clear();
        m_worlds.clear();

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
        // New loose entities are created in the active world.
        PrefabEditorEntityOwnershipService* ownershipService = GetActiveWorldOwnershipService();
        if (!ownershipService)
        {
            AZ_Warning("EditorEntityContextComponent", false, "Cannot create entity '%s': the active world has no ownership service.", name);
            return AZ::EntityId();
        }

        AZ::Entity* entity = aznew AZ::Entity(name);
        AZ_Assert(entity != nullptr, "Entity with name %s couldn't be created.", name);
        ownershipService->AddEntity(entity);
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
        PrefabEditorEntityOwnershipService* ownershipService = GetActiveWorldOwnershipService();
        if (!ownershipService)
        {
            AZ_Warning("EditorEntityContextComponent", false, "Cannot create entity '%s': the active world has no ownership service.", name);
            return AZ::EntityId();
        }

        entity = aznew AZ::Entity(entityId, name);
        AZ_Assert(entity != nullptr, "Entity with name %s couldn't be created.", name);
        ownershipService->AddEntity(entity);
        FinalizeEditorEntity(entity);

        return entity->GetId();
    }

    PrefabEditorEntityOwnershipService* EditorEntityContextComponent::GetActiveWorldOwnershipService()
    {
        return static_cast<PrefabEditorEntityOwnershipService*>(
            GetWorldEntityOwnershipService(AzFramework::EntityContextId::CreateNull()));
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
        // Entities register with the context of the world whose prefab instance owns them.
        AZStd::unordered_map<AzFramework::EntityContextId, EntityList> entitiesPerWorld;
        for (AZ::Entity* entity : entities)
        {
            entitiesPerWorld[FindEntityWorldId(entity->GetId())].push_back(entity);
        }
        for (auto& [worldId, worldEntities] : entitiesPerWorld)
        {
            auto worldIt = m_worlds.find(worldId);
            AzFramework::EntityOwnershipService* ownershipService =
                worldIt != m_worlds.end() ? worldIt->second->GetOwnershipService() : m_entityOwnershipService.get();
            ownershipService->HandleEntitiesAdded(worldEntities);
        }
    }

    AzFramework::EntityContextId EditorEntityContextComponent::FindEntityWorldId(AZ::EntityId entityId)
    {
        auto* instanceMapper = AZ::Interface<Prefab::InstanceEntityMapperInterface>::Get();
        Prefab::InstanceOptionalReference owningInstance =
            instanceMapper ? instanceMapper->FindOwningInstance(entityId) : AZStd::nullopt;
        if (!owningInstance.has_value())
        {
            return GetContextId();
        }

        const Prefab::Instance* rootInstance = &owningInstance->get();
        while (rootInstance->GetParentInstance().has_value())
        {
            rootInstance = &rootInstance->GetParentInstance()->get();
        }

        for (const auto& [worldId, world] : m_worlds)
        {
            PrefabEditorEntityOwnershipInterface* worldService = world->GetOwnershipService();
            Prefab::InstanceOptionalReference worldRoot = worldService->GetRootPrefabInstance();
            if (worldRoot.has_value() && &worldRoot->get() == rootInstance)
            {
                return worldId;
            }
        }
        return GetContextId();
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

        // Game mode plays the world being edited, and must stop the same one even if the focus moves.
        m_playingWorldId = GetActiveWorldId();

        auto* service = GetWorldEntityOwnershipService(m_playingWorldId);
        AZ_Assert(service, "Start play in editor could not start because there was no implementation for "
            "PrefabEditorEntityOwnershipInterface");
        service->StartPlayInEditor();

        m_isRunningGame = true;

        // Game mode is editor-wide: every world but the one playing goes quiet. Their entities would
        // otherwise keep rendering and debug-drawing into the main scene the session spawns into.
        if (m_playingWorldId != GetContextId())
        {
            GetWorldEntityOwnershipService(GetContextId())->SuspendEditorEntities();
        }
        for (const auto& [worldId, world] : m_worlds)
        {
            if (worldId != m_playingWorldId)
            {
                world->GetOwnershipService()->SuspendEditorEntities();
            }
        }

        RebindViewportsShowingWorld(m_playingWorldId);

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

        // A stop may arrive without a matching start; that addresses world 0, as it always did.
        const AzFramework::EntityContextId playedWorldId = m_playingWorldId.IsNull() ? GetContextId() : m_playingWorldId;
        m_playingWorldId = AzFramework::EntityContextId::CreateNull();

        // The world that started game mode can be gone by now - its viewport may have been closed while
        // the game was running - so the editor entities still have to be resumed either way.
        if (auto* service = GetWorldEntityOwnershipService(playedWorldId))
        {
            service->StopPlayInEditor();
        }
        else
        {
            AZ_Warning(
                "EditorEntityContextComponent", false,
                "Stop play in editor could not complete: the world that started game mode no longer exists.");
        }

        if (playedWorldId != GetContextId())
        {
            if (auto* editorWorldService = GetWorldEntityOwnershipService(GetContextId()))
            {
                editorWorldService->ResumeEditorEntities();
            }
        }
        for (const auto& [worldId, world] : m_worlds)
        {
            if (worldId != playedWorldId)
            {
                world->GetOwnershipService()->ResumeEditorEntities();
            }
        }

        RebindViewportsShowingWorld(playedWorldId);

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

        return contextId == GetContextId() || m_worlds.find(contextId) != m_worlds.end();
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

    AzFramework::EntityContextId EditorEntityContextComponent::LoadWorld(AZ::IO::PathView levelPrefabPath)
    {
        auto* prefabLoader = AZ::Interface<Prefab::PrefabLoaderInterface>::Get();
        AZ_Assert(prefabLoader, "LoadWorld called without a prefab loader");
        if (!prefabLoader)
        {
            return AzFramework::EntityContextId::CreateNull();
        }
        const AZ::IO::Path relativePath = prefabLoader->GenerateRelativePath(levelPrefabPath);

        // A level is never loaded into two worlds: the editor's own level is world 0, and a world
        // already showing the level is returned as-is.
        if (auto* ownService = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get())
        {
            Prefab::InstanceOptionalReference rootInstance = ownService->GetRootPrefabInstance();
            if (rootInstance.has_value() && rootInstance->get().GetTemplateSourcePath() == relativePath)
            {
                return GetContextId();
            }
        }

        for (const auto& [existingWorldId, world] : m_worlds)
        {
            if (world->GetLevelPath() == relativePath)
            {
                return existingWorldId;
            }
        }

        auto world = AZStd::make_unique<EditorWorld>(*this, levelPrefabPath);
        if (!world->IsValid())
        {
            return AzFramework::EntityContextId::CreateNull();
        }

        const AzFramework::EntityContextId worldId = world->GetContextId();
        m_worlds.emplace(worldId, AZStd::move(world));
        return worldId;
    }

    void EditorEntityContextComponent::BindViewportToWorld(
        AzFramework::ViewportId viewportId, const AzFramework::EntityContextId& worldId)
    {
        const AzFramework::EntityContextId previousWorldId = GetViewportWorld(viewportId);

        if (worldId.IsNull() || worldId == GetContextId())
        {
            m_viewportWorlds.erase(viewportId);
        }
        else
        {
            m_viewportWorlds[viewportId] = worldId;
        }

        // A world whose last viewport unbound is torn down.
        if (!previousWorldId.IsNull() && previousWorldId != GetContextId() && previousWorldId != worldId)
        {
            const bool worldStillBound = AZStd::any_of(
                m_viewportWorlds.begin(), m_viewportWorlds.end(),
                [&previousWorldId](const auto& binding)
                {
                    return binding.second == previousWorldId;
                });
            if (!worldStillBound)
            {
                // Notify before the erase: handlers clean up state keyed by this world, and once it is
                // gone its id no longer resolves, so that cleanup would be misattributed to world 0.
                EditorEntityContextNotificationBus::Broadcast(
                    &EditorEntityContextNotification::OnWorldDestroyed, previousWorldId);

                if (m_playingWorldId == previousWorldId)
                {
                    // Game mode outlived the world that started it, so StopPlayInEditor must not go
                    // looking for an ownership service that no longer exists.
                    m_playingWorldId = AzFramework::EntityContextId::CreateNull();
                }

                m_worlds.erase(previousWorldId);
            }
        }

        const AzFramework::EntityContextId newWorldId = GetViewportWorld(viewportId);
        if (newWorldId != previousWorldId)
        {
            EditorEntityContextNotificationBus::Broadcast(
                &EditorEntityContextNotification::OnViewportWorldChanged, viewportId, newWorldId);

            // The viewport's synchronous rebind above attached a render scene to the world's scene;
            // the world's level instantiates only now so its entities resolve there, not to Main.
            if (auto worldIt = m_worlds.find(newWorldId); worldIt != m_worlds.end() && worldIt->second->LoadPendingLevel())
            {
                EditorEntityContextNotificationBus::Broadcast(
                    &EditorEntityContextNotification::OnWorldLoaded, newWorldId);
            }

            if (viewportId == m_focusedViewportId)
            {
                EditorEntityContextNotificationBus::Broadcast(
                    &EditorEntityContextNotification::OnActiveWorldChanged, previousWorldId, newWorldId);
            }
        }
    }

    AzFramework::EntityContextId EditorEntityContextComponent::GetViewportWorld(AzFramework::ViewportId viewportId)
    {
        auto viewportWorldIt = m_viewportWorlds.find(viewportId);
        return viewportWorldIt != m_viewportWorlds.end() ? viewportWorldIt->second : GetContextId();
    }

    AZStd::string EditorEntityContextComponent::GetWorldLevelPath(const AzFramework::EntityContextId& worldId)
    {
        auto worldIt = m_worlds.find(worldId);
        return worldIt != m_worlds.end() ? worldIt->second->GetLevelPath().Native() : AZStd::string();
    }

    void EditorEntityContextComponent::SaveWorlds()
    {
        auto* prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        if (!prefabPublicInterface)
        {
            return;
        }

        for (const auto& [worldId, world] : m_worlds)
        {
            const AZ::IO::Path& levelPath = world->GetLevelPath();

            const auto unsavedChanges = prefabPublicInterface->HasUnsavedChanges(levelPath);
            if (!unsavedChanges.IsSuccess() || !unsavedChanges.GetValue())
            {
                continue;
            }

            const auto saveResult = prefabPublicInterface->SavePrefab(levelPath);
            AZ_Error(
                "EditorWorld", saveResult.IsSuccess(), "Could not save the level '%s': %s", levelPath.c_str(),
                saveResult.GetError().c_str());
        }
    }

    void EditorEntityContextComponent::RebindViewportsShowingWorld(const AzFramework::EntityContextId& worldId)
    {
        // World 0 renders in the main scene whether or not it is playing, so it never needs rebinding;
        // only viewports bound to a secondary world do, and those are exactly the map's entries.
        if (worldId == GetContextId())
        {
            return;
        }

        for (const auto& [viewportId, boundWorldId] : m_viewportWorlds)
        {
            if (boundWorldId == worldId)
            {
                // The binding does not change; the scene GetWorldScene hands out for the world does.
                EditorEntityContextNotificationBus::Broadcast(
                    &EditorEntityContextNotification::OnViewportWorldChanged, viewportId, worldId);
            }
        }
    }

    AzFramework::EntityContextId EditorEntityContextComponent::GetActiveWorldId()
    {
        return GetViewportWorld(m_focusedViewportId);
    }

    void EditorEntityContextComponent::SetFocusedViewport(AzFramework::ViewportId viewportId)
    {
        const AzFramework::EntityContextId previousWorldId = GetActiveWorldId();
        m_focusedViewportId = viewportId;
        const AzFramework::EntityContextId newWorldId = GetActiveWorldId();
        if (newWorldId != previousWorldId)
        {
            EditorEntityContextNotificationBus::Broadcast(
                &EditorEntityContextNotification::OnActiveWorldChanged, previousWorldId, newWorldId);
        }
    }

    PrefabEditorEntityOwnershipInterface* EditorEntityContextComponent::GetWorldEntityOwnershipService(
        const AzFramework::EntityContextId& worldId)
    {
        const AzFramework::EntityContextId resolvedWorldId = ResolveWorldId(worldId);

        if (resolvedWorldId == GetContextId())
        {
            return static_cast<PrefabEditorEntityOwnershipService*>(m_entityOwnershipService.get());
        }

        auto worldIt = m_worlds.find(resolvedWorldId);
        return worldIt != m_worlds.end() ? worldIt->second->GetOwnershipService() : nullptr;
    }

    AZStd::shared_ptr<AzFramework::Scene> EditorEntityContextComponent::GetWorldScene(const AzFramework::EntityContextId& worldId)
    {
        const AzFramework::EntityContextId resolvedWorldId = ResolveWorldId(worldId);

        // The played world's entities are respawned into the main scene for the session.
        if (resolvedWorldId == GetContextId() || resolvedWorldId == m_playingWorldId)
        {
            // World 0 renders in the main scene, as the stock viewport always bound.
            auto sceneSystem = AzFramework::SceneSystemInterface::Get();
            return sceneSystem ? sceneSystem->GetScene(AzFramework::Scene::MainSceneName) : nullptr;
        }

        auto worldIt = m_worlds.find(resolvedWorldId);
        return worldIt != m_worlds.end() ? worldIt->second->GetScene() : nullptr;
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
