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
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>

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

        virtual void CreateMenu();

        virtual void CreateTabBar();
        virtual void AddTabForDocumentId(const AZ::Uuid& documentId);
        virtual void RemoveTabForDocumentId(const AZ::Uuid& documentId);
        virtual void UpdateTabForDocumentId(const AZ::Uuid& documentId);
        virtual AZ::Uuid GetDocumentIdFromTab(const int tabIndex) const;

        virtual void OpenTabContextMenu();
        virtual void SelectPreviousTab();
        virtual void SelectNextTab();

        AzQtComponents::FancyDocking* m_advancedDockManager = nullptr;
        QWidget* m_centralWidget = nullptr;
        QMenuBar* m_menuBar = nullptr;
        AzQtComponents::TabWidget* m_tabWidget = nullptr;
        QStatusBar* m_statusBar = nullptr;

        AZStd::unordered_map<AZStd::string, AzQtComponents::StyledDockWidget*> m_dockWidgets;
    };
} // namespace AtomToolsFramework
