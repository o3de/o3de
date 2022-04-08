/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindow.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#endif

class QMenu;

namespace AtomToolsFramework
{
    //! AtomToolsDocumentMainWindow is a bridge between the base main window class and the document system. It has actions and menus for
    //! operations like creating, opening, saving, and closing documents. Additionally, it automatically manages tabs and views for each
    //! open document.
    class AtomToolsDocumentMainWindow
        : public AtomToolsMainWindow
        , private AtomToolsDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AtomToolsDocumentMainWindow, AZ::SystemAllocator, 0);

        using Base = AtomToolsMainWindow;

        AtomToolsDocumentMainWindow(const AZ::Crc32& toolId, const QString& objectName, QWidget* parent = 0);
        ~AtomToolsDocumentMainWindow();

        //! Helper function to get the absolute path for a document represented by the document ID
        QString GetDocumentPath(const AZ::Uuid& documentId) const;

        //! Retrieves the document ID from a tab with the index
        AZ::Uuid GetDocumentTabId(const int tabIndex) const;

        //! Retrieves the document ID from the currently selected tab
        AZ::Uuid GetCurrentDocumentId() const;

        //! Searches for the tab index corresponding to the document ID, returning -1 if not found
        int GetDocumentTabIndex(const AZ::Uuid& documentId) const;

        //! Determine if a tab exists for the document ID
        bool HasDocumentTab(const AZ::Uuid& documentId) const;

        //! If one does not already exist, this creates a new tab for a document using the file name as the label and full path as the
        //! tooltip
        bool AddDocumentTab(const AZ::Uuid& documentId, QWidget* viewWidget);

        //! Destroys the tab and view associated with the document ID
        void RemoveDocumentTab(const AZ::Uuid& documentId);

        //! Updates the displayed text and tooltip for a tab associated with a document ID
        void UpdateDocumentTab(const AZ::Uuid& documentId);

        //! Select the document tab to the left of the current document tab. If the first document is selected then the selection wraps
        //! around to the last one.
        void SelectPrevDocumentTab();

        //! Select the document tab to the right of the current documents tab. If the last document is selected then the selection wraps
        //! around to the first one.
        void SelectNextDocumentTab();

        //! Forces a context menu to appear above the selected tab, populated with actions for the associated document ID
        virtual void OpenDocumentTabContextMenu();

        //! Insert items into the tab context menu for the document ID
        virtual void PopulateTabContextMenu(const AZ::Uuid& documentId, QMenu& menu);

        //! Requests a source and target path for creating a new document based on another
        virtual bool GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath);

        //! Prompts the user for a selection of documents to open
        virtual AZStd::vector<AZStd::string> GetOpenDocumentParams();

        // AtomToolsMainWindowRequestBus::Handler overrides...
        void CreateMenus(QMenuBar* menuBar) override;
        void UpdateMenus(QMenuBar* menuBar) override;

    protected:
        void AddDocumentTabBar();

        void AddRecentFilePath(const AZStd::string& absolutePath);
        void ClearRecentFilePaths();
        void UpdateRecentFileMenu();

        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentCleared(const AZ::Uuid& documentId) override;
        void OnDocumentError(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;
        void OnDocumentUndoStateChanged(const AZ::Uuid& documentId) override;
        void OnDocumentSaved(const AZ::Uuid& documentId) override;

        void closeEvent(QCloseEvent* closeEvent) override;

        template<typename Functor>
        QAction* CreateAction(const QString& text, Functor functor, const QKeySequence& shortcut = 0);

        QMenu* m_menuOpenRecent = {};

        QAction* m_actionNew = {};
        QAction* m_actionOpen = {};
        QAction* m_actionClose = {};
        QAction* m_actionCloseAll = {};
        QAction* m_actionCloseOthers = {};
        QAction* m_actionSave = {};
        QAction* m_actionSaveAsCopy = {};
        QAction* m_actionSaveAsChild = {};
        QAction* m_actionSaveAll = {};

        QAction* m_actionUndo = {};
        QAction* m_actionRedo = {};

        QAction* m_actionNextTab = {};
        QAction* m_actionPreviousTab = {};

        AzQtComponents::TabWidget* m_tabWidget = {};

        static constexpr const char* RecentFilePathsKey = "/O3DE/AtomToolsFramework/Document/RecentFilePaths";
    };
} // namespace AtomToolsFramework
