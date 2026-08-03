/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Visibility/EntityVisibilityBoundsUnionSystem.h>

#include <AzToolsFramework/Entity/EditorEntityContextPickingBus.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include "EditorEntityContextBus.h"

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    class EntityContext;
}

namespace AzToolsFramework
{
    class PrefabEditorEntityOwnershipService;

    namespace UndoSystem
    {
        class UndoCacheInterface;
    }

    /**
     * System component responsible for owning the edit-time entity context.
     *
     * The editor entity context owns entities in the world, within the editor.
     * These entities typically own components inheriting from EditorComponentBase.
     */
    class AZTF_API EditorEntityContextComponent
        : public AZ::Component
        , public AzFramework::EntityContext
        , private EditorEntityContextRequestBus::Handler
        , private EditorEntityContextPickingRequestBus::Handler
        , private EditorLegacyGameModeNotificationBus::Handler
    {
    public:

        AZ_COMPONENT(EditorEntityContextComponent, "{92E3029B-6F4E-4628-8306-45D51EE59B8C}");

        EditorEntityContextComponent();
        ~EditorEntityContextComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // Component overrides
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorEntityContextRequestBus
        AzFramework::EntityContextId GetEditorEntityContextId() override { return GetContextId(); }
        AzFramework::EntityContext* GetEditorEntityContextInstance() override { return this; }
        void ResetEditorContext() override;

        AZ::EntityId CreateNewEditorEntity(const char* name) override;
        AZ::EntityId CreateNewEditorEntityWithId(const char* name, const AZ::EntityId& entityId) override;
        void AddEditorEntity(AZ::Entity* entity) override;
        void AddEditorEntities(const EntityList& entities) override;
        void HandleEntitiesAdded(const EntityList& entities) override;
        void FinalizeEditorEntity(AZ::Entity* entity) override;
        bool CloneEditorEntities(const EntityIdList& sourceEntities, 
                                 EntityList& resultEntities,
                                 AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) override;
        bool DestroyEditorEntity(AZ::EntityId entityId) override;

        bool SaveToStreamForEditor(
            AZ::IO::GenericStream& stream,
            const EntityList& entitiesInLayers,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& instancesInLayers) override;

        void GetLooseEditorEntities(EntityList& entityList) override;
        
        bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) override;
        bool LoadFromStream(AZ::IO::GenericStream& stream) override;
        bool LoadFromStreamWithLayers(AZ::IO::GenericStream& stream, QString levelPakFile) override;

        void StartPlayInEditor() override;
        void StopPlayInEditor() override;

        bool IsEditorRunningGame() override;
        bool IsEditorRequestingGame() override;
        bool IsEditorEntity(AZ::EntityId id) override;
        
        void AddRequiredComponents(AZ::Entity& entity) override;
        const AZ::ComponentTypeList& GetRequiredComponentTypes() override;

        bool MapEditorIdToRuntimeId(const AZ::EntityId& editorId, AZ::EntityId& runtimeId) override;
        bool MapRuntimeIdToEditorId(const AZ::EntityId& runtimeId, AZ::EntityId& editorId) override;

        AzFramework::EntityContextId LoadWorld(AZ::IO::PathView levelPrefabPath) override;
        void BindViewportToWorld(AzFramework::ViewportId viewportId, const AzFramework::EntityContextId& worldId) override;
        AzFramework::EntityContextId GetViewportWorld(AzFramework::ViewportId viewportId) override;
        AzFramework::EntityContextId GetActiveWorldId() override;
        void SetFocusedViewport(AzFramework::ViewportId viewportId) override;
        PrefabEditorEntityOwnershipInterface* GetWorldEntityOwnershipService(const AzFramework::EntityContextId& worldId) override;
        AZStd::shared_ptr<AzFramework::Scene> GetWorldScene(const AzFramework::EntityContextId& worldId) override;
        AZStd::string GetWorldLevelPath(const AzFramework::EntityContextId& worldId) override;
        void SaveWorlds() override;
        AZStd::vector<Prefab::TemplateId> GetModifiedWorldTemplateIds() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorEntityContextPickingRequestBus
        bool SupportsViewportEntityIdPicking() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityContext
        void PrepareForContextReset() override;
        bool ValidateEntitiesAreValidForContext(const EntityList& entities) override;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("EditorEntityContextService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("EditorEntityContextService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
            dependent.push_back(AZ_CRC_CE("SliceSystemService"));
            dependent.push_back(AZ_CRC_CE("PrefabSystem"));
        }

    protected:
        void OnContextEntitiesAdded(const EntityList& entities) override;
        void OnContextEntityRemoved(const AZ::EntityId& id) override;

        void SetupEditorEntity(AZ::Entity* entity);
        void SetupEditorEntities(const EntityList& entities);

        void LoadFromStreamComplete(bool loadedSuccessfully);

    private:
        EditorEntityContextComponent(const EditorEntityContextComponent&) = delete;

        //! Returns the world whose root prefab instance owns the entity (world 0 when none does).
        AzFramework::EntityContextId FindEntityWorldId(AZ::EntityId entityId);

        //! Re-sends OnViewportWorldChanged to the world's viewports so they pick up its current scene.
        void RebindViewportsShowingWorld(const AzFramework::EntityContextId& worldId);

        // EditorLegacyGameModeNotificationBus ...
        void OnStartGameModeRequest() override;
        void OnStopGameModeRequest() override;

        //! Indicates whether or not the editor is simulating the game.
        bool m_isRunningGame;
        //! Indicates whether or not the editor has been requested to move to game mode.
        bool m_isRequestingGame = false;

        //! List of selected entities prior to entering game.
        EntityIdList m_selectedBeforeStartingGame;

        //! The world game mode is playing, so the matching stop reaches it even if the focus moved.
        AzFramework::EntityContextId m_playingWorldId = AzFramework::EntityContextId::CreateNull();

        //! Bidirectional mapping of runtime entity Ids to their editor counterparts (relevant during in-editor simulation).
        using EntityIdToEntityIdMap = AZStd::unordered_map<AZ::EntityId, AZ::EntityId>;

        EntityIdToEntityIdMap m_editorToRuntimeIdMap;
        EntityIdToEntityIdMap m_runtimeToEditorIdMap;

        //! Array of types of required components added to all Editor entities with
        //! EditorEntityContextRequestBus::Events::AddRequiredComponents()
        AZ::ComponentTypeList m_requiredEditorComponentTypes;

        UndoSystem::UndoCacheInterface* m_undoCacheInterface = nullptr;

        //! Editor worlds other than world 0, which is this component itself.
        class EditorWorld;

        //! The worlds holding unsaved changes, world 0 aside - the editor tracks its own level.
        AZStd::vector<EditorWorld*> ModifiedWorlds() const;
        AZStd::unordered_map<AzFramework::EntityContextId, AZStd::unique_ptr<EditorWorld>> m_worlds;
        AZStd::unordered_map<AzFramework::ViewportId, AzFramework::EntityContextId> m_viewportWorlds;
        AzFramework::ViewportId m_focusedViewportId = AzFramework::InvalidViewportId;
    };
} // namespace AzToolsFramework

