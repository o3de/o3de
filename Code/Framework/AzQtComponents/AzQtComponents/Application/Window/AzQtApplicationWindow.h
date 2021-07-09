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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING
#endif

namespace AzQtComponents
{
    /**
     * //! Base class for O3DE Tools Applications Windows. Its responsibility is limited to initializing and connecting
     * its panels, managing selection of assets, and performing high-level actions like saving. It contains...
     */
    class AZ_QT_COMPONENTS_API AzQtApplicationWindow
        : public AzQtComponents::DockMainWindow
    {
        Q_OBJECT
    public:
        AzQtApplicationWindow(QWidget* parent, const AZStd::string& objectName);

    protected:
        virtual void SetupMenu() {};
        virtual void SetupTabs() {};

        virtual void OpenTabContextMenu() {};

        void SelectPreviousTab();
        void SelectNextTab();

        AzQtComponents::FancyDocking* m_advancedDockManager = nullptr;
        QMenuBar* m_menuBar = nullptr;
        QWidget* m_centralWidget = nullptr;
        AzQtComponents::TabWidget* m_tabWidget = nullptr;

        QVBoxLayout* vl;

        QMenu* m_menuFile = {};
        QAction* m_actionOpen = {};
        QAction* m_actionOpenRecent = {};
        QAction* m_actionClose = {};
        QAction* m_actionCloseAll = {};
        QAction* m_actionCloseOthers = {};
        QAction* m_actionSave = {};
        QAction* m_actionSaveAsCopy = {};
        QAction* m_actionSaveAll = {};
        QAction* m_actionExit = {};

        QMenu* m_menuEdit = {};
        QAction* m_actionUndo = {};
        QAction* m_actionRedo = {};
        QAction* m_actionSettings = {};

        QMenu* m_menuView = {};
        QAction* m_actionAssetBrowser = {};
        QAction* m_actionPythonTerminal = {};
        QAction* m_actionNextTab = {};
        QAction* m_actionPreviousTab = {};

        QMenu* m_menuHelp = {};
        QAction* m_actionHelp = {};
        QAction* m_actionAbout = {};
    };
} // namespace ShaderManagementConsole
