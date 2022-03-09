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

        AtomToolsDocumentMainWindow(const AZ::Crc32& toolId, QWidget* parent = 0);
        ~AtomToolsDocumentMainWindow();

        //! Helper function to get the absolute path for a document represented by the document ID
        QString GetDocumentPath(const AZ::Uuid& documentId) const;

        //! Retrieves the document ID from a tab with the given index
        AZ::Uuid GetDocumentTabId(const int tabIndex) const;

        //! If one does not already exist, this creates a new tab for the specified document ID with the provided label and tooltip
        void AddDocumentTab(const AZ::Uuid& documentId, const AZStd::string& label, const AZStd::string& toolTip);

        //! Destroys the tab and view associated with the given document ID
        void RemoveDocumentTab(const AZ::Uuid& documentId);

        //! Updates the displayed text and tooltip for a tab associated with a given document ID
        void UpdateDocumentTab(const AZ::Uuid& documentId, const AZStd::string& label, const AZStd::string& toolTip, bool isModified);

        //! Select the document tab to the left of the current document tab. If the first document is selected then the selection wraps
        //! around to the last one.
        void SelectPrevDocumentTab();

        //! Select the document tab to the right of the current documents tab. If the last document is selected then the selection wraps
        //! around to the first one.
        void SelectNextDocumentTab();

        //! Creates a widget that will be displayed beneath the tab for the specified document
        virtual QWidget* CreateDocumentTabView(const AZ::Uuid& documentId);

        //! Forces a context menu to appear above the selected tab, populated with actions for the associated document ID
        virtual void OpenDocumentTabContextMenu();

        //! Requests a source and target path for creating a new document based on another
        virtual bool GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath);

        //! Prompts the user for a selection of documents to open
        virtual AZStd::vector<AZStd::string> GetOpenDocumentParams();

    protected:
        void AddDocumentMenus();
        void AddDocumentTabBar();

        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;
        void OnDocumentUndoStateChanged(const AZ::Uuid& documentId) override;
        void OnDocumentSaved(const AZ::Uuid& documentId) override;

        void closeEvent(QCloseEvent* closeEvent) override;

        template<typename Functor>
        QAction* CreateAction(const QString& text, Functor functor, const QKeySequence& shortcut = 0);

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
    };
} // namespace AtomToolsFramework
