/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowser.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

#include <QLabel>
#include <QTimer>

namespace AtomToolsFramework
{
    class AtomToolsMainWindow
        : public AzQtComponents::DockMainWindow
        , protected AtomToolsMainWindowRequestBus::Handler
    {
    public:
        using Base = AzQtComponents::DockMainWindow;

        AtomToolsMainWindow(const AZ::Crc32& toolId, QWidget* parent = 0);
        ~AtomToolsMainWindow();

    protected:
        void showEvent(QShowEvent* showEvent) override;
        void closeEvent(QCloseEvent* closeEvent) override;

        // AtomToolsMainWindowRequestBus::Handler overrides...
        void ActivateWindow() override;
        bool AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area, uint32_t orientation) override;
        void RemoveDockWidget(const AZStd::string& name) override;
        void SetDockWidgetVisible(const AZStd::string& name, bool visible) override;
        bool IsDockWidgetVisible(const AZStd::string& name) const override;
        AZStd::vector<AZStd::string> GetDockWidgetNames() const override;

        void SetStatusMessage(const QString& message);
        void SetStatusWarning(const QString& message);
        void SetStatusError(const QString& message);

        void AddCommonMenus();

        virtual void OpenSettings();
        virtual void OpenHelp();
        virtual void OpenAbout();

        virtual void SetupMetrics();
        virtual void UpdateMetrics();
        virtual void UpdateWindowTitle();

        const AZ::Crc32 m_toolId = {};

        QPointer<AzQtComponents::FancyDocking> m_advancedDockManager = {};
        AzQtComponents::WindowDecorationWrapper* m_mainWindowWrapper = {};

        bool m_shownBefore = {};

        QByteArray m_defaultWindowState;

        QLabel* m_statusMessage = {};
        QLabel* m_statusBarFps = {};
        QLabel* m_statusBarCpuTime = {};
        QLabel* m_statusBarGpuTime = {};
        QTimer m_metricsTimer;

        QMenu* m_menuFile = {};
        QMenu* m_menuEdit = {};
        QMenu* m_menuView = {};
        QMenu* m_menuHelp = {};

        AtomToolsFramework::AtomToolsAssetBrowser* m_assetBrowser = {};

        AZStd::unordered_map<AZStd::string, AzQtComponents::StyledDockWidget*> m_dockWidgets;
        AZStd::unordered_map<AZStd::string, QAction*> m_dockActions;
    };
} // namespace AtomToolsFramework
