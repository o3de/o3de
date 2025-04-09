/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowser.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/SettingsDialog/SettingsDialog.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzToolsFramework/UI/Logging/StyledTracePrintFLogPanel.h>

#include <QLabel>
#include <QTimer>

namespace AtomToolsFramework
{
    class AtomToolsMainWindow
        : public AzQtComponents::DockMainWindow
        , protected AtomToolsMainWindowRequestBus::Handler
        , protected AtomToolsMainMenuRequestBus::Handler
    {
    public:
        using Base = AzQtComponents::DockMainWindow;

        AtomToolsMainWindow(const AZ::Crc32& toolId, const QString& objectName, QWidget* parent = 0);
        ~AtomToolsMainWindow();

        // AtomToolsMainWindowRequestBus::Handler overrides...
        void ActivateWindow() override;
        bool AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area) override;
        void RemoveDockWidget(const AZStd::string& name) override;
        void SetDockWidgetVisible(const AZStd::string& name, bool visible) override;
        bool IsDockWidgetVisible(const AZStd::string& name) const override;
        AZStd::vector<AZStd::string> GetDockWidgetNames() const override;
        void QueueUpdateMenus(bool rebuildMenus) override;

        void SetStatusMessage(const AZStd::string& message) override;
        void SetStatusWarning(const AZStd::string& message) override;
        void SetStatusError(const AZStd::string& message) override;

        // AtomToolsMainMenuRequestBus::Handler overrides...
        void CreateMenus(QMenuBar* menuBar) override;
        void UpdateMenus(QMenuBar* menuBar) override;

        virtual void PopulateSettingsInspector(InspectorWidget* inspector) const;
        virtual void OpenSettingsDialog();
        virtual void OnSettingsDialogClosed();

        virtual AZStd::string GetHelpUrl() const;
        virtual void OpenHelpUrl();

        virtual void OpenAboutDialog();

    protected:
        void showEvent(QShowEvent* showEvent) override;
        void closeEvent(QCloseEvent* closeEvent) override;

        void BuildDockingMenu();
        void BuildLayoutsMenu();
        void BuildScriptsMenu();

        static constexpr const char* ToolLayoutSettingsKey = "/O3DE/AtomToolsFramework/MainWindow/Layouts/Tool";
        static constexpr const char* UserLayoutSettingsKey = "/O3DE/AtomToolsFramework/MainWindow/Layouts/User";
        using LayoutSettingsMap = AZStd::map<AZStd::string, AZStd::string>;

        void RestoreDefaultLayout();
        void RestoreSavedLayout();

        virtual void SetupMetrics();
        virtual void UpdateMetrics();
        virtual void UpdateWindowTitle();

        const AZ::Crc32 m_toolId = {};

        QPointer<AzQtComponents::FancyDocking> m_advancedDockManager = {};
        AzQtComponents::WindowDecorationWrapper* m_mainWindowWrapper = {};

        bool m_shownBefore = {};
        bool m_updateMenus = {};
        bool m_rebuildMenus = {};

        QByteArray m_defaultWindowState;

        QLabel* m_statusMessage = {};
        QLabel* m_statusBarFps = {};
        QLabel* m_statusBarCpuTime = {};
        QLabel* m_statusBarGpuTime = {};
        QTimer m_metricsTimer;

        QMenu* m_menuFile = {};
        QMenu* m_menuEdit = {};
        QMenu* m_menuView = {};
        QMenu* m_menuTools = {};
        QMenu* m_menuHelp = {};

        AtomToolsFramework::AtomToolsAssetBrowser* m_assetBrowser = {};
        AzToolsFramework::LogPanel::StyledTracePrintFLogPanel* m_logPanel = {};

        mutable AZStd::shared_ptr<DynamicPropertyGroup> m_applicationSettingsGroup;
        mutable AZStd::shared_ptr<DynamicPropertyGroup> m_assetBrowserSettingsGroup;
    };
} // namespace AtomToolsFramework
