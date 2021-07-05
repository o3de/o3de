/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/Document/MaterialDocumentNotificationBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <Atom/Window/MaterialEditorWindowRequestBus.h>
#include <Viewport/MaterialViewportWidget.h>
#include <Window/ToolBar/MaterialEditorToolBar.h>
#include <Window/StatusBar/StatusBarWidget.h>

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    class CScriptTermDialog;
}

namespace MaterialEditor
{
    /**
     * MaterialEditorWindow is the main class. Its responsibility is limited to initializing and connecting
     * its panels, managing selection of assets, and performing high-level actions like saving. It contains...
     * 1) MaterialBrowser        - The user browses for Material (.material) assets.
     * 2) MaterialViewport        - The user can see the selected Material applied to a model.
     * 3) MaterialPropertyInspector  - The user edits the properties of the selected Material.
     */
    class MaterialEditorWindow
        : public AzQtComponents::DockMainWindow
        , private MaterialEditorWindowRequestBus::Handler
        , private MaterialDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialEditorWindow, AZ::SystemAllocator, 0);

        MaterialEditorWindow(QWidget* parent = 0);
        ~MaterialEditorWindow();

    private:
        // MaterialEditorWindowRequestBus::Handler overrides...
        void ActivateWindow() override;
        bool AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area, uint32_t orientation) override;
        void RemoveDockWidget(const AZStd::string& name) override;
        void SetDockWidgetVisible(const AZStd::string& name, bool visible) override;
        bool IsDockWidgetVisible(const AZStd::string& name) const override;
        AZStd::vector<AZStd::string> GetDockWidgetNames() const override;

        void ResizeViewportRenderTarget(uint32_t width, uint32_t height) override;
        void LockViewportRenderTargetSize(uint32_t width, uint32_t height) override;
        void UnlockViewportRenderTargetSize() override;

        // MaterialDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;
        void OnDocumentUndoStateChanged(const AZ::Uuid& documentId) override;
        void OnDocumentSaved(const AZ::Uuid& documentId) override;

        void SetupMenu();

        void SetupTabs();
        void AddTabForDocumentId(const AZ::Uuid& documentId);
        void RemoveTabForDocumentId(const AZ::Uuid& documentId);
        void UpdateTabForDocumentId(const AZ::Uuid& documentId);
        QString GetDocumentPath(const AZ::Uuid& documentId) const;
        AZ::Uuid GetDocumentIdFromTab(const int tabIndex) const;

        void OpenTabContextMenu();
        void SelectPreviousTab();
        void SelectNextTab();

        void closeEvent(QCloseEvent* closeEvent) override;

        AzQtComponents::FancyDocking* m_advancedDockManager = nullptr;
        QWidget* m_centralWidget = nullptr;
        QMenuBar* m_menuBar = nullptr;
        AzQtComponents::TabWidget* m_tabWidget = nullptr;
        MaterialViewportWidget* m_materialViewport = nullptr;
        MaterialEditorToolBar* m_toolBar = nullptr;

        AZStd::unordered_map <AZStd::string, AzQtComponents::StyledDockWidget*> m_dockWidgets;

        QMenu* m_menuFile = {};
        QAction* m_actionNew = {};
        QAction* m_actionOpen = {};
        QAction* m_actionOpenRecent = {};
        QAction* m_actionClose = {};
        QAction* m_actionCloseAll = {};
        QAction* m_actionCloseOthers = {};
        QAction* m_actionSave = {};
        QAction* m_actionSaveAsCopy = {};
        QAction* m_actionSaveAsChild = {};
        QAction* m_actionSaveAll = {};
        QAction* m_actionExit = {};

        QMenu* m_menuEdit = {};
        QAction* m_actionUndo = {};
        QAction* m_actionRedo = {};
        QAction* m_actionSettings = {};

        QMenu* m_menuView = {};
        QAction* m_actionAssetBrowser = {};
        QAction* m_actionInspector = {};
        QAction* m_actionConsole = {};
        QAction* m_actionPythonTerminal = {};
        QAction* m_actionPerfMonitor = {};
        QAction* m_actionViewportSettings = {};
        QAction* m_actionNextTab = {};
        QAction* m_actionPreviousTab = {};

        QMenu* m_menuHelp = {};
        QAction* m_actionHelp = {};
        QAction* m_actionAbout = {};

        StatusBarWidget* m_statusBar = {};
    };
} // namespace MaterialEditor
