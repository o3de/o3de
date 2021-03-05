/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

#include <QWidget>
#endif

namespace AZ
{
    class SerializeContext;
}

namespace Ui
{
    class AssetEditorToolbar;
    class AssetEditorStatusBar;
    class AssetEditorHeader;
}

class QMenu;

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;

    namespace AssetEditor
    {
        class AssetEditorWidgetUserSettings
            : public AZ::UserSettings
        {
        public:
            AZ_RTTI(AssetEditorWidgetUserSettings, "{382FE424-4541-4D93-9BA4-DE17A6DF8676}", AZ::UserSettings);
            AZ_CLASS_ALLOCATOR(AssetEditorWidgetUserSettings, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            AssetEditorWidgetUserSettings();
            ~AssetEditorWidgetUserSettings() override = default;

            void AddRecentPath(const AZStd::string& recentPath);

            AZStd::string m_lastSavePath;
            AZStd::vector< AZStd::string > m_recentPaths;
        };

        /**
        * Provides ability to create, edit, and save reflected assets.
        */
        class AssetEditorWidget
            : public QWidget
            , private AZ::Data::AssetBus::Handler
            , private AzFramework::AssetCatalogEventBus::Handler
            , private AzToolsFramework::IPropertyEditorNotify
            , private AZ::SystemTickBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetEditorWidget, AZ::SystemAllocator, 0);

            explicit AssetEditorWidget(QWidget* parent = nullptr);
            ~AssetEditorWidget() override;

            void CreateAsset(AZ::Data::AssetType assetType);
            void OpenAsset(const AZ::Data::Asset<AZ::Data::AssetData> asset);
            void SetAsset(const AZ::Data::Asset<AZ::Data::AssetData> asset);

            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            bool IsDirty() const { return m_dirty; }
            bool TrySave(const AZStd::function<void()>& savedCallback);
            bool WaitingToSave() const;
            void SetCloseAfterSave();

        public Q_SLOTS:
            void OpenAssetWithDialog();
            void OpenAssetFromPath(const AZStd::string& fullPath);

            void SaveAsset();
            void SaveAssetAs();
            bool SaveAssetToPath(AZStd::string_view assetPath);
            void ExpandAll();
            void CollapseAll();

            void OnNewAsset();

        Q_SIGNALS:
            void OnAssetSavedSignal();
            void OnAssetSaveFailedSignal(const AZStd::string& error);
            void OnAssetRevertedSignal();
            void OnAssetRevertFailedSignal(const AZStd::string& error);
            void OnAssetOpenedSignal(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

        protected: // IPropertyEditorNotify
            void AfterPropertyModified(InstanceDataNode* /*node*/) override;
            void RequestPropertyContextMenu(InstanceDataNode*, const QPoint&) override;

            void BeforePropertyModified(InstanceDataNode* node) override;

            void SetPropertyEditingActive(InstanceDataNode* /*node*/) override {}
            void SetPropertyEditingComplete(InstanceDataNode* /*node*/) override {}
            void SealUndoStack() override {};

            void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        private:
            void DirtyAsset();

            void SetStatusText(const QString& assetStatus);
            void ApplyStatusText();
            void SetupHeader();

            QString m_queuedAssetStatus;

            void AddRecentPath(const AZStd::string& recentPath);
            void PopulateRecentMenu();

            void UpdateMenusOnAssetOpen();

            // AZ::SystemTickBus
            void OnSystemTick() override;

            AZStd::vector<AZ::Data::AssetType>  m_genericAssetTypes;
            AZ::Data::AssetId                    m_sourceAssetId;
            AZ::Data::Asset<AZ::Data::AssetData> m_inMemoryAsset;
            Ui::AssetEditorHeader* m_header;
            ReflectedPropertyEditor* m_propertyEditor;
            AZ::SerializeContext* m_serializeContext = nullptr;

            // Ids can change when an asset goes from in-memory to saved on disk.
            // If there is a failure, the asset will be removed from the catalog.
            // The only reliable mechanism to be certain the asset being added/removed 
            // from the catalog is the same one that was added is to compare its file path.
            AZStd::string m_expectedAddedAssetPath; 
            AZStd::string m_recentlyAddedAssetPath;

            bool m_dirty = false;
            
            QString m_currentAsset;

            QAction* m_saveAssetAction;
            QAction* m_saveAsAssetAction;

            QMenu* m_recentFileMenu;

            AZStd::vector<AZ::u8> m_saveData;
            bool m_closeAfterSave = false;
            bool m_waitingToSave = false;

            AZStd::intrusive_ptr<AssetEditorWidgetUserSettings> m_userSettings;
            AZStd::unique_ptr< Ui::AssetEditorStatusBar > m_statusBar;

            void PopulateGenericAssetTypes();
            void CreateAssetImpl(AZ::Data::AssetType assetType);
            bool SaveAsDialog(AZ::Data::Asset<AZ::Data::AssetData>& asset);
            bool SaveImpl(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const QString& saveAsPath);
            void LoadAsset(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType);
            void UpdatePropertyEditor(AZ::Data::Asset<AZ::Data::AssetData>& asset);

            void GenerateSaveDataSnapshot();
        };
    } // namespace AssetEditor
} // namespace AzToolsFramework
