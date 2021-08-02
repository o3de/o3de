/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
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
    {
    public:
        AtomToolsMainWindow(QWidget* parent = 0);
    protected:
        AzQtComponents::FancyDocking* m_advancedDockManager = nullptr;
        QWidget* m_centralWidget = nullptr;
        QMenuBar* m_menuBar = nullptr;
        AzQtComponents::TabWidget* m_tabWidget = nullptr;

        virtual void SetupMenu();

        virtual void SetupTabs();
        virtual void AddTabForDocumentId(const AZ::Uuid& documentId);
        virtual void RemoveTabForDocumentId(const AZ::Uuid& documentId);
        virtual void UpdateTabForDocumentId(const AZ::Uuid& documentId);
        virtual AZ::Uuid GetDocumentIdFromTab(const int tabIndex) const;
        
        virtual void OpenTabContextMenu();
        virtual void SelectPreviousTab();
        virtual void SelectNextTab();

        QMenu* m_menuFile = {};
    };
}
