/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/TickBus.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

#include <QWidget>
#include <QToolButton>
#endif

namespace AZ
{
    class SerializeContext;
    namespace DocumentPropertyEditor
    {
        class ReflectionAdapter;
    }
}

namespace AzQtComponents
{
    class TabWidget;
}

namespace Ui
{
    class AssetEditorToolbar;
    class AssetEditorStatusBar;
    class AssetEditorHeader;
} // namespace Ui

class QMenu;

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
    class DocumentPropertyEditor;
    class FilteredDPE;

    namespace AssetEditor
    {
        class AssetEditorTab;

        class AssetEditorWidgetUserSettings : public AZ::UserSettings
        {
        public:
            AZ_RTTI(AssetEditorWidgetUserSettings, "{382FE424-4541-4D93-9BA4-DE17A6DF8676}", AZ::UserSettings);
            AZ_CLASS_ALLOCATOR(AssetEditorWidgetUserSettings, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            AssetEditorWidgetUserSettings();
            ~AssetEditorWidgetUserSettings() override = default;

            void AddRecentPath(const AZStd::string& recentPath);

            AZStd::string m_lastSavePath;
            AZStd::vector<AZStd::string> m_recentPaths;
        };

        /**
         * Provides ability to create, edit, and save reflected assets.
         */
        class AssetEditorWidget
            : public QWidget
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetEditorWidget, AZ::SystemAllocator);

            explicit AssetEditorWidget(QWidget* parent = nullptr);
            ~AssetEditorWidget() override;

            void CreateAsset(AZ::Data::AssetType assetType, const AZ::Uuid& observerToken);
            void OpenAsset(const AZ::Data::Asset<AZ::Data::AssetData> asset);

            bool SaveAssetToPath(AZStd::string_view assetPath);
            void SaveAll();
            bool SaveAllAndClose();

            void SetStatusText(const QString& assetStatus);
            void UpdateSaveMenuActionsStatus();
            void SetCurrentTab(AssetEditorTab* tab);

            void UpdateTabTitle(AssetEditorTab* tab);
            void SetLastSavePath(const AZStd::string& savePath);
            const QString GetLastSavePath() const;
            void AddRecentPath(const AZStd::string& recentPath);

            void CloseTab(AssetEditorTab* tab);
            void CloseTabAndContainerIfEmpty(AssetEditorTab* tab);

            bool IsValidAssetType(const AZ::Data::AssetType& assetType) const;
            void CreateAsset(AZ::Data::AssetType assetType);

        public Q_SLOTS:
            void OpenAssetWithDialog();
            void OpenAssetFromPath(const AZStd::string& fullPath);
            void OnAssetSaveFailed(const AZStd::string& error);
            void OnAssetOpened(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

            void SaveAsset();
            void SaveAssetAs();
            void ExpandAll();
            void CollapseAll();

            void currentTabChanged(int newCurrentIndex);
            void onTabCloseButtonPressed(int tabIndexToClose);

        Q_SIGNALS:
            void OnAssetSaveFailedSignal(const AZStd::string& error);
            void OnAssetOpenedSignal(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

        protected: // IPropertyEditorNotify
            void UpdateRecentFileListState();

        private:
            AssetEditorTab* MakeNewTab(const QString& name);
            AssetEditorTab* FindTabForAsset(const AZ::Data::AssetId& assetId) const;
            void ShowAddAssetMenu(const QToolButton* menuButton);

            void PopulateRecentMenu();

            void CloseOnNextTick();

            AzQtComponents::TabWidget* m_tabs;
            AZStd::vector<AZ::Data::AssetType>  m_genericAssetTypes;
            AZ::Data::AssetId                    m_sourceAssetId;
            AZ::Data::Asset<AZ::Data::AssetData> m_inMemoryAsset;
            Ui::AssetEditorHeader* m_header = nullptr;
            ReflectedPropertyEditor* m_propertyEditor = nullptr;
            AZStd::shared_ptr<AZ::DocumentPropertyEditor::ReflectionAdapter> m_adapter;
            FilteredDPE* m_filteredWidget = nullptr;
            DocumentPropertyEditor* m_dpe = nullptr;
            AZ::SerializeContext* m_serializeContext = nullptr;

            // Ids can change when an asset goes from in-memory to saved on disk.
            // If there is a failure, the asset will be removed from the catalog.
            // The only reliable mechanism to be certain the asset being added/removed 
            // from the catalog is the same one that was added is to compare its file path.
            AZStd::string m_expectedAddedAssetPath; 
            AZStd::string m_recentlyAddedAssetPath;

            bool m_dirty = false;
            bool m_useDPE = false;
            
            QString m_currentAsset;

            QAction* m_saveAssetAction;
            QAction* m_saveAsAssetAction;
            QAction* m_saveAllAssetsAction;

            QMenu* m_recentFileMenu;
            QMenu* m_newAssetMenu = nullptr;

            unsigned int m_nextNewAssetIndex = 1;

            AZStd::intrusive_ptr<AssetEditorWidgetUserSettings> m_userSettings;
            AZStd::unique_ptr<Ui::AssetEditorStatusBar> m_statusBar;

            AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeEvent::Handler m_propertyChangeHandler;
            AZ::Crc32 m_savedStateKey;

            void PopulateGenericAssetTypes();
        };
    } // namespace AssetEditor
} // namespace AzToolsFramework
