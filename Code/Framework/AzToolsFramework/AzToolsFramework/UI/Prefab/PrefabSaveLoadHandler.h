/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UserSettings/UserSettings.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserSourceDropBus.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

#include <AzQtComponents/Components/Widgets/Card.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabLoaderInterface;
        class PrefabPublicInterface;
        class PrefabSystemComponentInterface;

        //! Structure for saving/retrieving user settings related to prefab workflows.
        class PrefabUserSettings : public AZ::UserSettings
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabUserSettings, AZ::SystemAllocator, 0);
            AZ_RTTI(PrefabUserSettings, "{E17A6128-E2C3-4501-B1AD-B8BB0D315602}", AZ::UserSettings);

            AZStd::string m_saveLocation;
            bool m_autoNumber =
                false; //!< Should the name of the prefab file be automatically numbered. e.g PrefabName_001.prefab vs PrefabName.prefab.

            PrefabUserSettings() = default;

            static void Reflect(AZ::ReflectContext* context);
        };

        //! Class to handle dialogs and windows related to prefab save operations.
        class PrefabSaveHandler final
            : public QObject
            , public AssetBrowser::AssetBrowserSourceDropBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabSaveHandler, AZ::SystemAllocator, 0);

            PrefabSaveHandler();
            ~PrefabSaveHandler();

            // Settings management
            static bool GetPrefabSaveLocation(AZStd::string& path, AZ::u32 settingsId);
            static void SetPrefabSaveLocation(const AZStd::string& path, AZ::u32 settingsId);

            // EntityOutlinerSourceDropHandlingBus overrides ...
            void HandleSourceFileType(AZStd::string_view sourceFilePath, AZ::EntityId parentId, AZ::Vector3 position) const override;

            // Dialogs
            int ExecuteClosePrefabDialog(TemplateId templateId);
            void ExecuteSavePrefabDialog(TemplateId templateId, bool useSaveAllPrefabsPreference);
            static bool QueryUserForPrefabFilePath(AZStd::string& outPrefabFilePath);
            static bool QueryUserForProceduralPrefabAsset(AZStd::string& outPrefabAssetPath);
            static bool QueryUserForPrefabSaveLocation(
                const AZStd::string& suggestedName,
                const char* initialTargetDirectory,
                AZ::u32 prefabUserSettingsId,
                QWidget* activeWindow,
                AZStd::string& outPrefabName,
                AZStd::string& outPrefabFilePath);

            // Path and filename generation
            enum class PrefabSaveResult
            {
                Continue,
                Retry,
                Cancel
            };

            static void GenerateSuggestedFilenameFromEntities(const EntityIdList& entities, AZStd::string& outName);
            static bool AppendEntityToSuggestedFilename(AZStd::string& filename, AZ::EntityId entityId);
            static PrefabSaveResult IsPrefabPathValidForAssets(QWidget* activeWindow, QString prefabPath, AZStd::string& retrySavePath);
            static void GenerateSuggestedPrefabPath(
                const AZStd::string& prefabName, const AZStd::string& targetDirectory, AZStd::string& suggestedFullPath);

        private:
            AZStd::shared_ptr<QDialog> ConstructClosePrefabDialog(TemplateId templateId);
            AZStd::unique_ptr<AzQtComponents::Card> ConstructUnsavedPrefabsCard(TemplateId templateId);
            AZStd::unique_ptr<QDialog> ConstructSavePrefabDialog(TemplateId templateId, bool useSaveAllPrefabsPreference);
            void SavePrefabsInDialog(QDialog* unsavedPrefabsDialog);

            static const AZStd::string s_prefabFileExtension;

            static PrefabLoaderInterface* s_prefabLoaderInterface;
            static PrefabPublicInterface* s_prefabPublicInterface;
            static PrefabSystemComponentInterface* s_prefabSystemComponentInterface;
        };
    }
}
