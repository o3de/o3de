/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentInspector.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsInspector.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportToolBar.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportWidget.h>
#include <AtomToolsFramework/Graph/GraphDocumentNotificationBus.h>
#include <AtomToolsFramework/Graph/GraphViewSettings.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>

#include <QAction>
#include <QMenu>
#include <QTabWidget>

namespace MaterialCanvas
{
    //! Material Canvas as an Editor view pane: the standalone tool's document logic and widgets without its application shell.
    class MaterialCanvasPaneWindow
        : public AzQtComponents::DockMainWindow
        , private AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        , private AtomToolsFramework::GraphDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialCanvasPaneWindow, AZ::SystemAllocator);

        using Base = AzQtComponents::DockMainWindow;

        //! Constructed by RegisterViewPane with a null parent and reparented afterwards, so never assume a parent.
        explicit MaterialCanvasPaneWindow(QWidget* parent = nullptr);
        ~MaterialCanvasPaneWindow() override;

        //! Adds a document view as a tab; the pane's equivalent of AtomToolsDocumentMainWindow::AddDocumentTab.
        bool AddDocumentView(const AZ::Uuid& documentId, QWidget* viewWidget);

        //! Writes the dock layout to the registry; public because the component must call it before Qt deletes this widget.
        void SaveLayout() const;

        //! Invoked by the Editor Action Manager Save action while this pane has focus.
        void SaveCurrentDocument();
        //! Invoked by the Editor Action Manager reroute action while this pane has focus.
        void AddRerouteToSelectedConnection();

    protected:
        // QWidget overrides...
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

    private:
        MaterialCanvasPaneWindow(const MaterialCanvasPaneWindow&) = delete;
        MaterialCanvasPaneWindow& operator=(const MaterialCanvasPaneWindow&) = delete;

        // AtomToolsDocumentNotificationBus::Handler overrides, matching AtomToolsDocumentMainWindow (incl. OnDocumentSaved).
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentSaved(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::GraphDocumentNotificationBus::Handler overrides...
        void OnCompileGraphCompleted(const AZ::Uuid& documentId) override;
        void OnDocumentUndoStateChanged(const AZ::Uuid& documentId) override;
        void OnDocumentCleared(const AZ::Uuid& documentId) override;
        void OnDocumentError(const AZ::Uuid& documentId) override;

        void CreateMenus();
        void CreateDocumentTabs();
        void CreateViewportDock();
        void CreateInspectorDock();
        void CreateNodePaletteDock();

        //! Docks @widget in a plain QDockWidget at @area; AtomToolsMainWindow::AddDockWidget without FancyDocking.
        QDockWidget* AddDock(const QString& name, QWidget* widget, Qt::DockWidgetArea area, bool visible = true);

        //! Restores the saveState layout from the settings registry, as Script Canvas does (no FancyDocking).
        void RestoreLayout();

        //! The settings dialog, reproduced from AtomToolsMainWindow so Material Canvas options are reachable from the pane.
        void OpenSettingsDialog();
        void PopulateSettingsInspector(AtomToolsFramework::InspectorWidget* inspector) const;

        void UpdateRecentFileMenu();

        //! Prompts to save a modified document. Returns false if the user cancelled, which aborts the close.
        bool CloseDocumentCheck(const AZ::Uuid& documentId);
        bool CloseDocuments(const AZStd::vector<AZ::Uuid>& documentIds);
        bool SaveDocument(const AZ::Uuid& documentId);

        AZStd::vector<AZ::Uuid> GetOpenDocumentIds() const;
        AZ::Uuid GetCurrentDocumentId() const;
        int GetTabIndexForDocument(const AZ::Uuid& documentId) const;
        void UpdateDocumentTab(const AZ::Uuid& documentId);
        void UpdateMenuState();

        AZ::Crc32 m_toolId;
        AtomToolsFramework::GraphViewSettingsPtr m_graphViewSettingsPtr;

        //! Must be declared after m_graphViewSettingsPtr: its constructor reads the style sheet path out of it.
        GraphCanvas::StyleManager m_styleManager;

        QTabWidget* m_tabWidget = {};
        AtomToolsFramework::AtomToolsDocumentInspector* m_documentInspector = {};
        AtomToolsFramework::EntityPreviewViewportToolBar* m_toolBar = {};
        AtomToolsFramework::EntityPreviewViewportWidget* m_materialViewport = {};
        AtomToolsFramework::EntityPreviewViewportSettingsInspector* m_viewportSettingsInspector = {};
        GraphCanvas::NodePaletteDockWidget* m_nodePalette = {};
        QDockWidget* m_inspectorDock = {};
        QDockWidget* m_viewportDock = {};

        QMenu* m_menuOpenRecent = {};
        // Publishes the material the graph describes; a copy of MaterialCanvasMainWindow's action, since the pane builds its own menu.
        QAction* m_actionApply = {};

        QAction* m_actionSave = {};
        QAction* m_actionSaveAs = {};
        QAction* m_actionSaveAll = {};
        QAction* m_actionClose = {};
        QAction* m_actionCloseAll = {};

        //! Rebuilt each time the settings dialog opens, hence mutable, matching MaterialCanvasMainWindow.
        mutable AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup> m_materialCanvasCompileSettingsGroup;
    };
} // namespace MaterialCanvas
