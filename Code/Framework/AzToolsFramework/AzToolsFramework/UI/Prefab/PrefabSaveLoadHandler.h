/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UserSettings/UserSettings.h>
namespace AZ
{
    class Vector3;
}

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

#include <AzQtComponents/Buses/DragAndDrop.h>
#include <AzQtComponents/Components/Widgets/Card.h>

class QMimeData;
class QDragEvent;
class QDropEvent;
class QDragMoveEvent;
class QDragLeaveEvent;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
    }

    class ToastNotificationsView;

    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class PrefabLoaderInterface;
        class PrefabPublicInterface;
        class PrefabSystemComponentInterface;
        class TemplateInstanceMapperInterface;
        

        //! Structure for saving/retrieving user settings related to prefab workflows.
        class PrefabUserSettings : public AZ::UserSettings
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabUserSettings, AZ::SystemAllocator);
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
            , public AzQtComponents::DragAndDropEventsBus::Handler
            , public AzQtComponents::DragAndDropItemViewEventsBus::Handler
            , private AzToolsFramework::AssetSystemBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabSaveHandler, AZ::SystemAllocator);

            PrefabSaveHandler();
            ~PrefabSaveHandler();

            // Settings management
            static bool GetPrefabSaveLocation(AZStd::string& path, AZ::u32 settingsId);
            static void SetPrefabSaveLocation(const AZStd::string& path, AZ::u32 settingsId);

            // Drag and drop overrides:
            int GetDragAndDropEventsPriority() const override;

            // DragAndDropEventsBus- dragging over widgets
            void DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
            void DragMove(QDragMoveEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
            void DragLeave(QDragLeaveEvent* event) override;
            void Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) override;

            // DragAndDropItemViewEventsBus - listview/outliner dragging:
            int GetDragAndDropItemViewEventsPriority() const override;
            void CanDropItemView(bool& accepted, AzQtComponents::DragAndDropContextBase& context) override;
            void DoDropItemView(bool& accepted, AzQtComponents::DragAndDropContextBase& context) override;

            //! Initializes PrefabToastNotificationsView member, which uses the editor's main window as the parent widget to show toasts.
            void InitializePrefabToastNotificationsView();

            // Dialogs
            int ExecuteClosePrefabDialog(TemplateId templateId);
            void ExecuteSavePrefabDialog(AZ::EntityId entityId);
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

            //! AssetSystemBus notification handlers
            //! @{
            //! Called by the AssetProcessor when the source file has been removed.
            void SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
            //! @}

            AZStd::shared_ptr<QDialog> ConstructClosePrefabDialog(TemplateId templateId);
            AZStd::unique_ptr<AzQtComponents::Card> ConstructUnsavedPrefabsCard(TemplateId templateId);
            AZStd::unique_ptr<QDialog> ConstructSavePrefabDialog(TemplateId templateId, bool useSaveAllPrefabsPreference);
            void SavePrefabsInDialog(QDialog* unsavedPrefabsDialog);

            static const AZStd::string s_prefabFileExtension;
            static const AZStd::string s_procPrefabFileExtension;

            static PrefabLoaderInterface* s_prefabLoaderInterface;
            static PrefabPublicInterface* s_prefabPublicInterface;
            static PrefabSystemComponentInterface* s_prefabSystemComponentInterface;

            AZStd::unique_ptr<AzToolsFramework::ToastNotificationsView> m_prefabToastNotificationsView;

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            TemplateInstanceMapperInterface* m_templateInstanceMapperInterface = nullptr;

            bool CanDragAndDropData(
                const QMimeData* data,
                AZStd::vector<AZStd::string>* prefabsToInstantiate = nullptr,
                AZStd::vector<AZStd::string>* prefabsToDetach = nullptr) const;

            void DoDragAndDropData(
                const AZ::Vector3& instantiateLocation,
                const AZ::EntityId& parentEntity,
                const AZStd::vector<AZStd::string>& prefabsToInstantiate,
                const AZStd::vector<AZStd::string>& prefabsToDetach);
        };
    }
}
