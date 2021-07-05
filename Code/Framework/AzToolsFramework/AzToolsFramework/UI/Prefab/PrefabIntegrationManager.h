/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserSourceDropBus.h>
#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/UI/Prefab/LevelRootUiHandler.h>
#include <AzToolsFramework/UI/Prefab/PrefabEditManager.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationBus.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabUiHandler.h>

namespace AzToolsFramework
{
    namespace Prefab
    {

        class PrefabLoaderInterface;

        //! Structure for saving/retrieving user settings related to prefab workflows.
        class PrefabUserSettings
            : public AZ::UserSettings
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabUserSettings, AZ::SystemAllocator, 0);
            AZ_RTTI(PrefabUserSettings, "{E17A6128-E2C3-4501-B1AD-B8BB0D315602}", AZ::UserSettings);

            AZStd::string m_saveLocation;
            bool m_autoNumber = false; //!< Should the name of the prefab file be automatically numbered. e.g PrefabName_001.prefab vs PrefabName.prefab.

            PrefabUserSettings() = default;

            static void Reflect(AZ::ReflectContext* context);
        };

        class PrefabIntegrationManager final
            : public EditorContextMenuBus::Handler
            , public AssetBrowser::AssetBrowserSourceDropBus::Handler
            , public PrefabInstanceContainerNotificationBus::Handler
            , public PrefabIntegrationInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabIntegrationManager, AZ::SystemAllocator, 0);

            PrefabIntegrationManager();
            ~PrefabIntegrationManager();

            static void Reflect(AZ::ReflectContext* context);

            // EditorContextMenuBus...
            int GetMenuPosition() const override;
            AZStd::string GetMenuIdentifier() const override;
            void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;

            // EntityOutlinerSourceDropHandlingBus...
            void HandleSourceFileType(AZStd::string_view sourceFilePath, AZ::EntityId parentId, AZ::Vector3 position) const override;

            // PrefabInstanceContainerNotificationBus...
            void OnPrefabComponentActivate(AZ::EntityId entityId) override;
            void OnPrefabComponentDeactivate(AZ::EntityId entityId) override;

            // PrefabIntegrationInterface...
            AZ::EntityId CreateNewEntityAtPosition(const AZ::Vector3& position, AZ::EntityId parentId) override;

        private:
            // Manages the Edit Mode UI for prefabs
            PrefabEditManager m_prefabEditManager;

            // Used to handle the UI for the level root
            LevelRootUiHandler m_levelRootUiHandler;

            // Used to handle the UI for prefab entities
            PrefabUiHandler m_prefabUiHandler;

            // Context menu item handlers
            static void ContextMenu_CreatePrefab(AzToolsFramework::EntityIdList selectedEntities);
            static void ContextMenu_InstantiatePrefab();
            static void ContextMenu_EditPrefab(AZ::EntityId containerEntity);
            static void ContextMenu_SavePrefab(AZ::EntityId containerEntity);
            static void ContextMenu_DeleteSelected();
            static void ContextMenu_DetachPrefab(AZ::EntityId containerEntity);

            // Prompt and resolve dialogs
            static bool QueryUserForPrefabSaveLocation(
                const AZStd::string& suggestedName, const char* initialTargetDirectory, AZ::u32 prefabUserSettingsId, QWidget* activeWindow,
                AZStd::string& outPrefabName, AZStd::string& outPrefabFilePath);
            static bool QueryUserForPrefabFilePath(AZStd::string& outPrefabFilePath);
            static void WarnUserOfError(AZStd::string_view title, AZStd::string_view message);

            // Path and filename generation
            static void GenerateSuggestedFilenameFromEntities(const EntityIdList& entities, AZStd::string& outName);
            static bool AppendEntityToSuggestedFilename(AZStd::string& filename, AZ::EntityId entityId);

            enum class PrefabSaveResult
            {
                Continue,
                Retry,
                Cancel
            };
            static PrefabSaveResult IsPrefabPathValidForAssets(QWidget* activeWindow, QString prefabPath, AZStd::string& retrySavePath);
            static void GenerateSuggestedPrefabPath(const AZStd::string& prefabName, const AZStd::string& targetDirectory, AZStd::string& suggestedFullPath);

            // Reference detection
            static void GatherAllReferencedEntitiesAndCompare(const EntityIdSet& entities, EntityIdSet& entitiesAndReferencedEntities,
                bool& hasExternalReferences);
            static void GatherAllReferencedEntities(EntityIdSet& entitiesWithReferences, AZ::SerializeContext& serializeContext);
            static bool QueryAndPruneMissingExternalReferences(EntityIdSet& entities, EntityIdSet& selectedAndReferencedEntities,
                bool& useReferencedEntities, bool defaultMoveExternalRefs = false);

            // Settings management
            static void SetPrefabSaveLocation(const AZStd::string& path, AZ::u32 settingsId);
            static bool GetPrefabSaveLocation(AZStd::string& path, AZ::u32 settingsId);

            static AZ::u32 GetSliceFlags(const AZ::Edit::ElementData* editData, const AZ::Edit::ClassData* classData);

            static const AZStd::string s_prefabFileExtension;

            static EditorEntityUiInterface* s_editorEntityUiInterface;
            static PrefabPublicInterface* s_prefabPublicInterface;
            static PrefabEditInterface* s_prefabEditInterface;
            static PrefabLoaderInterface* s_prefabLoaderInterface;
        };
    }
}
