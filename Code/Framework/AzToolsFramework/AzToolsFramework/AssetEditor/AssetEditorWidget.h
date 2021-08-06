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
#endif

namespace AZ
{
    class SerializeContext;
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

    namespace AssetEditor
    {
        class AssetEditorTab;

        class AssetEditorWidgetUserSettings : public AZ::UserSettings
        {
        public:
            AZ_RTTI(AssetEditorWidgetUserSettings, "{382FE424-4541-4D93-9BA4-DE17A6DF8676}", AZ::UserSettings);
            AZ_CLASS_ALLOCATOR(AssetEditorWidgetUserSettings, AZ::SystemAllocator, 0);

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
            , private AZ::SystemTickBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetEditorWidget, AZ::SystemAllocator, 0);

            explicit AssetEditorWidget(QWidget* parent = nullptr);
            ~AssetEditorWidget() override;

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

            void PopulateRecentMenu();

            // AZ::SystemTickBus
            void OnSystemTick() override;
            void CloseOnNextTick();

            AZStd::vector<AZ::Data::AssetType> m_genericAssetTypes;
            AZ::SerializeContext* m_serializeContext = nullptr;
            AzQtComponents::TabWidget* m_tabs;

            QAction* m_saveAssetAction;
            QAction* m_saveAsAssetAction;
            QAction* m_saveAllAssetsAction;

            QMenu* m_recentFileMenu;

            unsigned int m_nextNewAssetIndex = 1;

            AZStd::intrusive_ptr<AssetEditorWidgetUserSettings> m_userSettings;
            AZStd::unique_ptr<Ui::AssetEditorStatusBar> m_statusBar;

            void PopulateGenericAssetTypes();
        };
    } // namespace AssetEditor
} // namespace AzToolsFramework
