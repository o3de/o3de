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
    //! Material Canvas as an O3DE Editor view pane.
    //!
    //! WHAT THIS BORROWS FROM WHERE
    //!
    //! There are two distinct layers in the standalone tool, and only one of them belongs in a pane.
    //!
    //! AtomToolsMainWindow is the application shell: it wraps itself in a WindowDecorationWrapper to become a top-level
    //! window, runs its own AzQtComponents::FancyDocking instance, persists its own geometry, and builds an Asset Browser,
    //! Python Terminal and Log Panel of its own. The Editor already provides every one of those and owns placement itself,
    //! so none of that layer is used here. Docking is plain Qt, the way Script Canvas does it, and the Editor's own docking
    //! manages this window from outside. The one thing worth taking from that layer is the settings dialog, which is how
    //! every Material Canvas option is reached -- so OpenSettingsDialog and PopulateSettingsInspector are reproduced below.
    //!
    //! AtomToolsDocumentMainWindow is a different matter. Its tab bar, create/open/save menus, recent files, drag and drop,
    //! save prompts and document notification handling are pure document logic with nothing standalone-specific about them.
    //! Those behaviours follow the same implementations here rather than being reinvented.
    //!
    //! Nothing here is a fork of Material Canvas. The graph view, inspector, viewport, node palette and viewport content are
    //! the same public classes the standalone tool uses, assembled differently. MaterialCanvasApplication and
    //! MaterialCanvasMainWindow are untouched.
    class MaterialCanvasPaneWindow
        : public AzQtComponents::DockMainWindow
        , private AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        , private AtomToolsFramework::GraphDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialCanvasPaneWindow, AZ::SystemAllocator);

        using Base = AzQtComponents::DockMainWindow;

        //! Constructed by AzToolsFramework::RegisterViewPane, which always passes a null parent and reparents the finished
        //! widget into its own DockWidget afterwards. Nothing here may assume it has a parent.
        explicit MaterialCanvasPaneWindow(QWidget* parent = nullptr);
        ~MaterialCanvasPaneWindow() override;

        //! Adds a document view widget as a tab. Called by the document type view factories on
        //! MaterialCanvasEditorSystemComponent, the equivalent of AtomToolsDocumentMainWindow::AddDocumentTab.
        bool AddDocumentView(const AZ::Uuid& documentId, QWidget* viewWidget);

        //! Writes the current dock layout into the settings registry.
        //!
        //! Public because MaterialCanvasEditorSystemComponent has to call it while this widget is still alive. The destructor calls it
        //! as well, but at Editor shutdown CloseViewPane hands the widget to Qt to delete, and that can happen after the component has
        //! already deactivated and written the registry out to disk. Relying on the destructor alone lost the layout every time.
        void SaveLayout() const;

        //! Invoked by the Editor Action Manager Save action while this pane has focus.
        void SaveCurrentDocument();

    protected:
        // QWidget overrides...
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

    private:
        MaterialCanvasPaneWindow(const MaterialCanvasPaneWindow&) = delete;
        MaterialCanvasPaneWindow& operator=(const MaterialCanvasPaneWindow&) = delete;

        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        // Every notification that can change a tab's title or the enabled state of a menu action is handled, matching
        // AtomToolsDocumentMainWindow. Missing OnDocumentSaved is what leaves a saved document showing a modified marker.
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

        //! Adds @widget to a plain QDockWidget and docks it in @area. The local equivalent of
        //! AtomToolsMainWindow::AddDockWidget, minus the FancyDocking involvement.
        QDockWidget* AddDock(const QString& name, QWidget* widget, Qt::DockWidgetArea area, bool visible = true);

        //! QMainWindow::saveState/restoreState, persisted through the settings registry. This is how Script Canvas keeps its
        //! layout; AtomToolsMainWindow's equivalent goes through FancyDocking, which this window does not use.
        void RestoreLayout();

        //! The settings dialog, reproduced from AtomToolsMainWindow. Without it none of the Material Canvas options --
        //! including the shader build and preview pipeline toggles -- can be reached from the pane at all.
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
        // Publishes the material the graph currently describes. The pane builds its own File menu rather than inheriting
        // AtomToolsDocumentMainWindow's, so this is a second copy of the action added to MaterialCanvasMainWindow.
        QAction* m_actionApply = {};

        QAction* m_actionSave = {};
        QAction* m_actionSaveAs = {};
        QAction* m_actionSaveAll = {};
        QAction* m_actionClose = {};
        QAction* m_actionCloseAll = {};

        //! Rebuilt each time the settings dialog opens, so it is mutable for the const populate function, matching
        //! MaterialCanvasMainWindow.
        mutable AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup> m_materialCanvasCompileSettingsGroup;
    };
} // namespace MaterialCanvas
