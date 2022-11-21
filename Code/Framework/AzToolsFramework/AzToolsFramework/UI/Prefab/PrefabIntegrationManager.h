/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzToolsFramework/UI/Prefab/LevelRootUiHandler.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationBus.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabSaveLoadHandler.h>
#include <AzToolsFramework/UI/Prefab/PrefabUiHandler.h>
#include <AzToolsFramework/UI/Prefab/Procedural/ProceduralPrefabReadOnlyHandler.h>
#include <AzToolsFramework/UI/Prefab/Procedural/ProceduralPrefabUiHandler.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class ContainerEntityInterface;
    class ReadOnlyEntityPublicInterface;
    class ToolBarManagerInterface;

    namespace Prefab
    {
        class PrefabFocusInterface;
        class PrefabFocusPublicInterface;
        class PrefabLoaderInterface;
        class PrefabOverridePublicInterface;
        class PrefabPublicInterface;

        class PrefabIntegrationManager final
            : public EditorContextMenuBus::Handler
            , public EditorEventsBus::Handler
            , public PrefabInstanceContainerNotificationBus::Handler
            , public PrefabIntegrationInterface
            , private PrefabFocusNotificationBus::Handler
            , private PrefabPublicNotificationBus::Handler
            , private EditorEntityContextNotificationBus::Handler
            , private ActionManagerRegistrationNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabIntegrationManager, AZ::SystemAllocator, 0);

            PrefabIntegrationManager();
            ~PrefabIntegrationManager();

            static void Reflect(AZ::ReflectContext* context);

            // EditorContextMenuBus overrides ...
            int GetMenuPosition() const override;
            AZStd::string GetMenuIdentifier() const override;
            void PopulateEditorGlobalContextMenu(QMenu* menu, const AZStd::optional<AzFramework::ScreenPoint>& point, int flags) override;

            // EditorEventsBus overrides ...
            void OnEscape() override;

            // EditorEntityContextNotificationBus overrides ...
            void OnStartPlayInEditorBegin() override;
            void OnStopPlayInEditor() override;

            // PrefabFocusNotificationBus overrides ...
            void OnPrefabFocusChanged(AZ::EntityId previousContainerEntityId, AZ::EntityId newContainerEntityId) override;
            void OnPrefabFocusRefreshed() override;

            // PrefabInstanceContainerNotificationBus overrides ...
            void OnPrefabComponentActivate(AZ::EntityId entityId) override;
            void OnPrefabComponentDeactivate(AZ::EntityId entityId) override;

            // PrefabIntegrationInterface overrides ...
            AZ::EntityId CreateNewEntityAtPosition(const AZ::Vector3& position, AZ::EntityId parentId) override;
            int HandleRootPrefabClosure(TemplateId templateId) override;
            void SaveCurrentPrefab() override;

            // ActionManagerRegistrationNotificationBus overrides ...
            void OnActionUpdaterRegistrationHook() override;
            void OnActionRegistrationHook() override;
            void OnWidgetActionRegistrationHook() override;
            void OnToolBarBindingHook() override;

        private:
            // PrefabPublicNotificationBus overrides ...
            void OnRootPrefabInstanceLoaded() override;

            // Handles the UI for prefab save operations.
            PrefabSaveHandler m_prefabSaveHandler;

            // Used to handle the UI for the level root.
            LevelRootUiHandler m_levelRootUiHandler;

            // Used to handle the UI for prefab entities.
            PrefabUiHandler m_prefabUiHandler;

            // Used to handle the UI for procedural prefab entities.
            ProceduralPrefabUiHandler m_proceduralPrefabUiHandler;

            // Ensures entities owned by procedural prefab instances are marked as read-only correctly.
            ProceduralPrefabReadOnlyHandler m_proceduralPrefabReadOnlyHandler;

            // Context menu item handlers
            void ContextMenu_CreatePrefab(AzToolsFramework::EntityIdList selectedEntities);
            void ContextMenu_InstantiatePrefab();
            void ContextMenu_InstantiateProceduralPrefab();
            void ContextMenu_ClosePrefab();
            void ContextMenu_EditPrefab(AZ::EntityId containerEntity);
            void ContextMenu_SavePrefab(AZ::EntityId containerEntity);
            void ContextMenu_ClosePrefabInstance(AZ::EntityId containerEntity);
            void ContextMenu_OpenPrefabInstance(AZ::EntityId containerEntity);
            void ContextMenu_Duplicate();
            void ContextMenu_DeleteSelected();
            void ContextMenu_DetachPrefab(AZ::EntityId containerEntity);
            void ContextMenu_RevertOverrides(AZ::EntityId entityId);

            // Shortcut setup handlers (for legacy action manager)
            void InitializeShortcuts();
            void UninitializeShortcuts();

            // Reference detection
            static void GatherAllReferencedEntitiesAndCompare(
                const EntityIdSet& entities, EntityIdSet& entitiesAndReferencedEntities, bool& hasExternalReferences);
            static void GatherAllReferencedEntities(EntityIdSet& entitiesWithReferences, AZ::SerializeContext& serializeContext);
            static bool QueryAndPruneMissingExternalReferences(
                EntityIdSet& entities,
                EntityIdSet& selectedAndReferencedEntities,
                bool& useReferencedEntities,
                bool defaultMoveExternalRefs = false);

            static AZ::u32 GetSliceFlags(const AZ::Edit::ElementData* editData, const AZ::Edit::ClassData* classData);

            AZStd::vector<AZStd::unique_ptr<QAction>> m_actions;

            static AzFramework::EntityContextId s_editorEntityContextId;

            static ContainerEntityInterface* s_containerEntityInterface;
            static EditorEntityUiInterface* s_editorEntityUiInterface;
            static PrefabFocusInterface* s_prefabFocusInterface;
            static PrefabFocusPublicInterface* s_prefabFocusPublicInterface;
            static PrefabLoaderInterface* s_prefabLoaderInterface;
            static PrefabPublicInterface* s_prefabPublicInterface;

            
            ActionManagerInterface* m_actionManagerInterface = nullptr;
            PrefabOverridePublicInterface* m_prefabOverridePublicInterface = nullptr;
            ReadOnlyEntityPublicInterface* m_readOnlyEntityPublicInterface = nullptr;
            ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;
        };
    }
}
