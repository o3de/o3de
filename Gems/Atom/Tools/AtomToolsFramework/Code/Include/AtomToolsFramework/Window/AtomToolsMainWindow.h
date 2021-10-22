/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <QLabel>

namespace AtomToolsFramework
{
    class AtomToolsMainWindow
        : public AzQtComponents::DockMainWindow
        , protected AtomToolsMainWindowRequestBus::Handler
    {
    public:
        AtomToolsMainWindow(QWidget* parent = 0);
        ~AtomToolsMainWindow();

    protected:
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

        AzQtComponents::FancyDocking* m_advancedDockManager = {};

        QLabel* m_statusMessage = {};

        QMenu* m_menuFile = {};
        QMenu* m_menuEdit = {};
        QMenu* m_menuView = {};
        QMenu* m_menuHelp = {};

        AZStd::unordered_map<AZStd::string, AzQtComponents::StyledDockWidget*> m_dockWidgets;
        AZStd::unordered_map<AZStd::string, QAction*> m_dockActions;
    };
} // namespace AtomToolsFramework
